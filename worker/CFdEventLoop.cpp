/*
 * Copyright (C) 2015   Jeremy Chen jeremy_cz@yahoo.com
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <algorithm>
#include <stdlib.h>
#include <utils/Log.h>
#include <common_base/CFdEventLoop.h>
#include <common_base/CSysFdWatch.h>
#include <common_base/CBaseWorker.h>

CSysFdWatch::CSysFdWatch(int fd, int32_t flags)
    : mFd(fd)
    , mFlags(flags)
    , mEnable(false)
    , mEventLoop(0)
{}

CSysFdWatch::~CSysFdWatch()
{
    if (mFd > 0)
    {
        closePollFd(mFd);
    }
}

void CSysFdWatch::enable(bool enb)
{
    mEventLoop->enableWatch(this, enb);
    mEnable = enb;
}

class CNotifyFdWatch : public CSysFdWatch
{
public:
    CNotifyFdWatch(CBaseWorker *worker)
        : CSysFdWatch(-1, POLLIN | POLLERR | POLLHUP)
        , mWorker(worker)
    {}
protected:
    void onInput(bool &io_error)
    {
        if (mEventLoop->mEventFd.pickEvent())
        {
            mEventLoop->mMutex.lock();
            mWorker->processJobQueue(); // mutex will be unlocked
        }
        else
        {
            //TODO: do something???
        }
    }
    void onHup()
    {
        mWorker->doExit();
        LOG_E("CFdEventLoop: Worker exits due to eventfd error!\n");
    }
    void onError()
    {
        mWorker->doExit();
        LOG_E("CFdEventLoop: Worker exits due to eventfd hup!\n");
    }
private:
    CBaseWorker *mWorker;
};

CFdEventLoop::CFdEventLoop()
    : mWatchBlackList(0)
    , mNotifyWatch(0)
    , mRebuildPollFd(false)
{
}

CFdEventLoop::~CFdEventLoop()
{
    uninstallWatches();
    if (mNotifyWatch)
    {
        delete mNotifyWatch;
    }
}

bool CFdEventLoop::watchDestroyed(CSysFdWatch *watch)
{
    if (mWatchBlackList && (mWatchBlackList->find(watch) != mWatchBlackList->end()))
    {
        LOG_I("CFdEventLoop: watch is destroyed inside callback.\n");
        return true;
    }
    return false;
}

void CFdEventLoop::addWatchToBlacklist(CSysFdWatch *watch)
{
    if (mWatchBlackList)
    {
        mWatchBlackList->insert(watch);
    }
}

void CFdEventLoop::buildFdArray()
{
    if (!mRebuildPollFd)
    {
        return;
    }
    mRebuildPollFd = false;
    mPollFds.clear();
    mPollWatches.clear();

    for (tCFdWatchList::iterator wi = mWatchWorkingList.begin(); wi != mWatchWorkingList.end(); ++wi)
    {
        int fd = (*wi)->descriptor();
        if (fd < 0)
        {
            LOG_E("CFdEventLoop: Bad file descriptor: %d!\n", fd);
            continue;
        }

        pollfd pfd;
        pfd.fd = fd;
        pfd.events = (int16_t)(*wi)->flags();
        pfd.revents = 0;
        mPollFds.push_back(pfd);

        mPollWatches.push_back(*wi);
    }
}

void CFdEventLoop::processWatches()
{
    tWatchTbl watch_black_list;
    enableWatchBlackList(&watch_black_list);
    /*
     * Since the first fd is for job processing and might delete other watches,
     * handle it at last.
     */
    tWatchPollTbl::reverse_iterator wit;
    tFdPollTbl::reverse_iterator fdit;
    for (wit = mPollWatches.rbegin(), fdit = mPollFds.rbegin();
           wit != mPollWatches.rend(); ++wit, ++fdit)
    {
        CSysFdWatch *w = *wit;
        if (watchDestroyed(w))
        {
            continue;
        }
        int32_t events = w->convertRetEvents(fdit->revents);
        fdit->revents = 0;
        if (events & (POLLIN | POLLOUT | POLLERR | POLLHUP))
        {
            bool io_error = false;
            if (events & POLLERR)
            {
                try
                {
                    w->onError();
                }
                catch (...)
                {
                    LOG_E("CFdEventLoop: Exception received at line %d of file %s!\n", __LINE__, __FILE__);
                    if (!watchDestroyed(w))
                    {
                        removeWatch(w);
                        delete w;
                    }
                }
                continue;
            }
            if (events & POLLHUP)
            {
                try
                {
                    w->onHup();
                }
                catch (...)
                {
                    LOG_E("CFdEventLoop: Exception received at line %d of file %s!\n", __LINE__, __FILE__);
                }
                continue;
            }
            if (events & POLLIN)
            {
                try
                {
                    w->onInput(io_error);
                }
                catch (...)
                {
                    LOG_E("CFdEventLoop: Exception received at line %d of file %s!\n", __LINE__, __FILE__);
                }
                if (watchDestroyed(w))
                {
                    continue;
                }
            }
            
            if ((events & POLLOUT) && !io_error)
            {
                try
                {
                    w->onOutput(io_error);
                }
                catch (...)
                {
                    LOG_E("CFdEventLoop: Exception received at line %d of file %s!\n", __LINE__, __FILE__);
                }
                if (watchDestroyed(w))
                {
                    continue;
                }
            }
            if (io_error)
            {
                try
                {
                    w->onError();
                }
                catch (...)
                {
                    LOG_E("CFdEventLoop: Exception received at line %d of file %s!\n", __LINE__, __FILE__);
                    if (!watchDestroyed(w))
                    {
                        removeWatch(w);
                        delete w;
                    }
                }
            }
        }
    }
    enableWatchBlackList(0);
}

void CFdEventLoop::dispatch()
{
    buildFdArray();
    if (!mPollFds.size())
    {
        LOG_E("CFdEventLoop: no watch fds enabled!\n");
        // avoid exhaustive of CPU power
        sysdep_sleep(LOOP_DEFAULT_INTERVAL);
        return;
    }

    int32_t wait_time = getMostRecentTime();
    int ret = poll(mPollFds.data(), mPollFds.size(), wait_time);
    if (ret == 0) // timeout
    {
        processTimers();
    }
    else if (ret > 0) // watch ready
    {
        processWatches();
    }
    else
    {
        LOG_E("CFdEventLoop: Error polling!\n");
        // avoid exhaustive of CPU power
        sysdep_sleep(LOOP_DEFAULT_INTERVAL);
    }
}

void CFdEventLoop::addWatch(CSysFdWatch *watch, bool enb)
{
    registerWatch(watch, true);
    watch->eventloop(this);
    watch->enable(enb);
}

void CFdEventLoop::removeWatch(CSysFdWatch *watch)
{
    addWatchToBlacklist(watch);
    registerWatch(watch, false);
    watch->enable(false);
    watch->eventloop(0);
}

bool CFdEventLoop::addWatchToList(tCFdWatchList &wlist, CSysFdWatch *watch, bool enable)
{
    bool did = false;
    tCFdWatchList::iterator wi = std::find(wlist.begin(), wlist.end(), watch);
    if (enable)
    {
        if (wi == wlist.end())
        {
            wlist.push_back(watch);
            did = true;
        }
    }
    else
    {
        if (wi != wlist.end())
        {
            wlist.remove(watch);
            did = true;
        }
    }
    return did;
}

bool CFdEventLoop::registerWatch(CSysFdWatch *watch, bool enable)
{
    return addWatchToList(mWatchList, watch, enable);
}

bool CFdEventLoop::enableWatch(CSysFdWatch *watch, bool enable)
{
    mRebuildPollFd = true;
    return addWatchToList(mWatchWorkingList, watch, enable);
}

void CFdEventLoop::uninstallWatches()
{
    for (CFdEventLoop::tCFdWatchList::iterator wi = mWatchList.begin(); wi != mWatchList.end();)
    {
    	CSysFdWatch *watch = *wi;
        ++wi;
        removeWatch(watch);
    }
}

bool CFdEventLoop::notify()
{
    return mEventFd.triggerEvent();
}

bool CFdEventLoop::init(CBaseWorker *worker)
{
    if (!mNotifyWatch)
    {
        int efd = -1;
        if (!mEventFd.create(efd))
        {
            return false;
        }

        mNotifyWatch = new CNotifyFdWatch(worker);
        mNotifyWatch->descriptor(efd);
        addWatch(mNotifyWatch, true);
    }
    return true;
}
