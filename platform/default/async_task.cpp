#include <mbgl/util/async_task.hpp>

#include <mbgl/util/run_loop.hpp>
#include <mbgl/util/uv_detail.hpp>

namespace mbgl {
namespace util {

class AsyncTask::Impl {
public:
    Impl(std::function<void()>&& fn)
        : async(RunLoop::getLoop(), std::move(fn)) {
    }

    uv::async async;
};

AsyncTask::AsyncTask(std::function<void()>&& fn)
    : impl(std::make_unique<Impl>(std::move(fn))) {
}

AsyncTask::~AsyncTask() {
}

void AsyncTask::send() {
    impl->async.send();
}

void AsyncTask::unref() {
    impl->async.unref();
}

}
}
