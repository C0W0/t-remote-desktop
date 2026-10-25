#include "MetalFrame.h"

#import <Metal/Metal.h>
#include <expected>
#include <memory>
#include <print>
#import <QuartzCore/CAMetalLayer.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_metal.h>

#include <cstdio>

namespace renderer {
class MetalRenderer::Impl {
public:
private:
  id<MTLDevice> device_ = nil;
  id<MTLCommandQueue> commandQueue_ = nil;
  SDL_MetalView metalView_ = nullptr; // Non-owning pointer to the Metal view

  friend class MetalRenderer;
};

MetalRenderer::MetalRenderer() : impl_(std::make_unique<Impl>()) {}
MetalRenderer::MetalRenderer(MetalRenderer&&) noexcept = default;
MetalRenderer& MetalRenderer::operator=(MetalRenderer&&) noexcept = default;

std::expected<MetalRenderer, std::string> MetalRenderer::Create(frame::MetalWindow &window) {
  auto impl = std::make_unique<MetalRenderer::Impl>();
  impl->metalView_ = window.metalView();

  auto* layer = (__bridge CAMetalLayer*)SDL_Metal_GetLayer(impl->metalView_);
  if (layer == nil) {
    return std::unexpected{"Failed to get CAMetalLayer from SDL_MetalView."};
  }

  id<MTLDevice> device = MTLCreateSystemDefaultDevice();
  if (!device) {
    return std::unexpected{"Metal is not supported on this device."};
  }

  impl->device_ = device;
  impl->commandQueue_ = [device newCommandQueue];

  if (impl->commandQueue_ == nil) {
    return std::unexpected{"Failed to create MTLCommandQueue."};
  }

  layer.device = impl->device_;
  layer.pixelFormat = MTLPixelFormatBGRA8Unorm;
  layer.framebufferOnly = YES;

  MetalRenderer renderer;
  renderer.impl_ = std::move(impl);
  return renderer;
}

void MetalRenderer::renderFrame() {
  auto* layer = (__bridge CAMetalLayer*)SDL_Metal_GetLayer(impl_->metalView_);

  @autoreleasepool {
    id<CAMetalDrawable> drawable = [layer nextDrawable];
    if (!drawable) {
      std::println("Failed to get CAMetalDrawable from CAMetalLayer.");
      return;
    }

    MTLRenderPassDescriptor* renderPassDescriptor = [MTLRenderPassDescriptor renderPassDescriptor];
    renderPassDescriptor.colorAttachments[0].texture = drawable.texture;
    renderPassDescriptor.colorAttachments[0].loadAction = MTLLoadActionClear;
    renderPassDescriptor.colorAttachments[0].storeAction = MTLStoreActionStore;
    renderPassDescriptor.colorAttachments[0].clearColor = MTLClearColorMake(0.08, 0.25, 0.55, 1.0);

    id<MTLCommandBuffer> commandBuffer = [impl_->commandQueue_ commandBuffer];
    id<MTLRenderCommandEncoder> renderEncoder = [commandBuffer renderCommandEncoderWithDescriptor:renderPassDescriptor];
    [renderEncoder endEncoding];
    [commandBuffer presentDrawable:drawable];
    [commandBuffer commit];
  }
}

MetalRenderer::~MetalRenderer() = default;
} // namespace renderer