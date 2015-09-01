#include <mbgl/util/run_loop.hpp>

namespace mbgl {
namespace util {

uv::tls<RunLoop> RunLoop::current;

class RunLoop::Impl {
public:
    Impl() = default;

    uv::loop loop;
};

RunLoop::RunLoop() : impl(std::make_unique<Impl>()) {
    current.set(this);
    async = std::make_unique<AsyncTask>(std::bind(&RunLoop::process, this));
}

RunLoop::~RunLoop() {
    current.set(nullptr);
}

LOOP_HANDLE RunLoop::getLoopHandle() {
    return current.get()->impl->loop.get();
}

void RunLoop::run() {
    impl->loop.run();
}

void RunLoop::stop() {
    invoke([&] { async->unref(); });
}

}
}
