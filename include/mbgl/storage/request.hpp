#ifndef MBGL_STORAGE_DEFAULT_REQUEST
#define MBGL_STORAGE_DEFAULT_REQUEST

#include <mbgl/storage/resource.hpp>

#include <mbgl/util/async_task.hpp>
#include <mbgl/util/noncopyable.hpp>
#include <mbgl/util/util.hpp>

#include <mutex>
#include <thread>
#include <functional>
#include <memory>

namespace mbgl {

class Response;

class Request : private util::noncopyable {
public:
    using Callback = std::function<void(const Response &)>;
    Request(const Resource &resource, Callback callback);

public:
    // May be called from any thread.
    void notify(const std::shared_ptr<const Response> &response);
    void destruct();

    // May be called only from the thread the Request was created in.
    void cancel();

private:
    ~Request();
    void invoke();
    void notifyCallback();

private:
    util::AsyncTask async;
    struct Canceled;
    std::unique_ptr<Canceled> canceled;
    Callback callback;
    std::shared_ptr<const Response> response;

public:
    const Resource resource;
};

}

#endif
