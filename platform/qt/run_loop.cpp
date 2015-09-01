#include <mbgl/util/run_loop.hpp>

#include <QEventLoop>

namespace mbgl {
namespace util {

uv::tls<RunLoop> RunLoop::current;

class RunLoop::Impl {
public:
    Impl() = default;

    QEventLoop loop;
};

RunLoop::RunLoop() : impl(std::make_unique<Impl>()) {
    current.set(this);
    async = std::make_unique<AsyncTask>(std::bind(&RunLoop::process, this));
}

RunLoop::~RunLoop() {
    current.set(nullptr);
}

LOOP_HANDLE RunLoop::getLoopHandle() {
    return &(current.get()->impl->loop);
}

void RunLoop::run() {
    impl->loop.exec();
}

void RunLoop::stop() {
    invoke([&] { impl->loop.exit(); });
}

}
}
