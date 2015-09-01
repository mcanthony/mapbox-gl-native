#include <mbgl/util/async_task.hpp>

#include "async_task_p.hpp"

#include <mbgl/util/run_loop.hpp>

namespace mbgl {
namespace util {

AsyncTask::Impl::Impl(std::function<void()>&& fn)
    : task(std::move(fn)), queued(ATOMIC_FLAG_INIT) {
    connect(this, SIGNAL(send(void)), this, SLOT(runTask(void)), Qt::QueuedConnection);
}

void AsyncTask::Impl::maySend() {
    if (!queued.test_and_set()) {
        emit send();
    }
}

void AsyncTask::Impl::runTask() {
    queued.clear();
    task();
}

AsyncTask::AsyncTask(std::function<void()>&& fn)
    : impl(std::make_unique<Impl>(std::move(fn))) {
}

AsyncTask::~AsyncTask() {
}

void AsyncTask::send() {
    impl->maySend();
}

void AsyncTask::unref() {
    // FIXME: Implement, so we don't block at exit.
}

}
}
