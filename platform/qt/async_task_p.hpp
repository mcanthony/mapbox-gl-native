#include <mbgl/util/async_task.hpp>

#include <QObject>

#include <functional>
#include <atomic>

namespace mbgl {
namespace util {

class AsyncTask::Impl : public QObject {
    Q_OBJECT

public:
    Impl(std::function<void()>&& fn);

    void maySend();

public slots:
    void runTask();

signals:
    void send();

private:
    std::function<void()> task;
    std::atomic_flag queued;
};


}
}
