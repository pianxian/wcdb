/*
 * Tencent is pleased to support the open source community by making
 * WCDB available.
 *
 * Copyright (C) 2017 THL A29 Limited, a Tencent company.
 * All rights reserved.
 *
 * Licensed under the BSD 3-Clause License (the "License"); you may not use
 * this file except in compliance with the License. You may obtain a copy of
 * the License at
 *
 *       https://opensource.org/licenses/BSD-3-Clause
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _WCDB_HANDLEPOOL_HPP
#define _WCDB_HANDLEPOOL_HPP

#include <WCDB/ErrorProne.hpp>
#include <WCDB/Lock.hpp>
#include <WCDB/RecyclableHandle.hpp>
#include <WCDB/ThreadedErrors.hpp>
#include <list>

namespace WCDB {

/*
 * There are two kind of locks in the pool.
 * 1. Memory lock is to protect the memory order of the variable inside the pool.
 * 2. Concurrency lock is to blockade all other operations while closing.
 * Corcurrency is always locked before Memory.
 *
 * When you are writing and reading any variables, you should lock or shared lock memory.
 * When you are operating m_handles, you should lock or shared lock concurrency in addition.
 */

class HandlePool : public ThreadedErrorProne {
#pragma mark - Initialize
public:
    HandlePool() = delete;
    HandlePool(const HandlePool &) = delete;
    HandlePool &operator=(const HandlePool &) = delete;

    HandlePool(const String &path);
    const String path;

    virtual ~HandlePool();

#pragma mark - Concurrency
public:
    void blockade();
    bool isBlockaded() const;
    void unblockade();

    typedef std::function<void(void)> DrainedCallback;
    void drain(const HandlePool::DrainedCallback &onDrained);

protected:
    mutable SharedLock m_concurrency;

private:
    static int maxHandleCount();
    bool allowedHandleCount();
    void clearAllHandles();

#pragma mark - Handle
public:
    typedef int Slot;
    RecyclableHandle flowOut(Slot slot);
    void purge();
    size_t aliveHandleCount() const;
    size_t activeHandleCount(Slot slot) const;

protected:
    virtual std::shared_ptr<Handle> generateHandle(Slot slot) = 0;
    virtual bool willConfigureHandle(Slot slot, Handle *handle) = 0;

    mutable SharedLock m_memory;

    const std::set<std::shared_ptr<Handle>> &getAllHandles(Slot slot);

private:
    void flowBack(Slot slot, const std::shared_ptr<Handle> &handle);

    std::map<Slot, std::set<std::shared_ptr<Handle>>> m_handles;
    std::map<Slot, std::list<std::shared_ptr<Handle>>> m_frees;
};

} //namespace WCDB

#endif /* _WCDB_HANDLEPOOL_HPP */
