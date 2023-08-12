#include <libcamera/libcamera.h>

#include <iomanip>
#include <iostream>
#include <memory>
#include <thread>

#include <libcamera/framebuffer_allocator.h>

using namespace libcamera;
using namespace std::chrono_literals;

static std::shared_ptr<Camera> camera;

static void requestComplete(Request* request) {
    if (request->status() == Request::RequestCancelled) {
        return;
    }

    const std::map<const Stream*, FrameBuffer*>& buffers = request->buffers();

    for (auto bufferPair : buffers) {
        FrameBuffer* buffer = bufferPair.second;
        const FrameMetadata& metadata = buffer->metadata();

        std::cout << " seq: " << std::setw(6) << std::setfill('0')
                  << metadata.sequence << " bytesused: ";

        unsigned int nplane = 0;
        for (const FrameMetadata::Plane& plane : metadata.planes()) {
            std::cout << plane.bytesused;
            if (++nplane < metadata.planes().size()) {
                std::cout << "/";
            }
        }

        std::cout << std::endl;
    }

    request->reuse(Request::ReuseBuffers);
    camera->queueRequest(request);
}

int main() {
    std::unique_ptr<CameraManager> cm = std::make_unique<CameraManager>();
    cm->start();

    if (cm->cameras().empty()) {
        std::cout << "No cameras were identified on the system." << std::endl;
        cm->stop();
        return EXIT_FAILURE;
    }

    std::string cameraId = cm->cameras()[0]->id();
    camera = cm->get(cameraId);

    /*
     * Note that is equivalent to:
     * camera = cm->cameras()[0];
     */

    std::unique_ptr<CameraConfiguration> config =
        camera->generateConfiguration({StreamRole::Viewfinder});

    StreamConfiguration& streamConfig = config->at(0);
    std::cout << "Default viewfinder configuration is: "
              << streamConfig.toString() << std::endl;

    streamConfig.size.width = 640;
    streamConfig.size.height = 480;

    config->validate();
    std::cout << "Validated viewfinder configuration is: "
              << streamConfig.toString() << std::endl;

    camera->configure(config.get());

    // FrameBufferAllocator* allocator = new FrameBufferAllocator(camera);
    std::unique_ptr<FrameBufferAllocator> allocator =
        std::make_unique<FrameBufferAllocator>(camera);

    for (StreamConfiguration& cfg : *config) {
        int ret = allocator->allocate(cfg.stream());
        if (ret < 0) {
            std::cerr << "Can't allocate buffers" << std::endl;
            return -ENOMEM;
        }

        size_t allocated = allocator->buffers(cfg.stream()).size();
        std::cout << "Allocated " << allocated << " buffers for stream"
                  << std::endl;
    }

    Stream* stream = streamConfig.stream();
    const std::vector<std::unique_ptr<FrameBuffer>>& buffers =
        allocator->buffers(stream);
    std::vector<std::unique_ptr<Request>> requests;

    for (const auto& buffer : buffers) {
        std::unique_ptr<Request> request = camera->createRequest();
        if (!request) {
            std::cerr << "Can't create request" << std::endl;
            return -ENOMEM;
        }

        int ret = request->addBuffer(stream, buffer.get());
        if (ret < 0) {
            std::cerr << "Can't set buffer for request" << std::endl;
            return ret;
        }

        requests.push_back(std::move(request));
    }

    camera->requestCompleted.connect(requestComplete);

    camera->start();
    for (std::unique_ptr<Request>& request : requests) {
        camera->queueRequest(request.get());
    }

    std::this_thread::sleep_for(3000ms);

    camera->stop();
    allocator->free(stream);
    // delete allocator;
    camera->release();
    camera.reset();
    cm->stop();

    return 0;
}

