/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
                2020 Vladimír Vondruš <mosra@centrum.cz>

    Permission is hereby granted, free of charge, to any person obtaining a
    copy of this software and associated documentation files (the "Software"),
    to deal in the Software without restriction, including without limitation
    the rights to use, copy, modify, merge, publish, distribute, sublicense,
    and/or sell copies of the Software, and to permit persons to whom the
    Software is furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
    THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
    DEALINGS IN THE SOFTWARE.
*/

#include <Corrade/Containers/Reference.h>

#include "Magnum/Vk/FramebufferCreateInfo.h"
#include "Magnum/Vk/ImageCreateInfo.h"
#include "Magnum/Vk/ImageViewCreateInfo.h"
#include "Magnum/Vk/RenderPassCreateInfo.h"
#include "Magnum/Vk/Result.h"
#include "Magnum/Vk/VulkanTester.h"

namespace Magnum { namespace Vk { namespace Test { namespace {

struct FramebufferVkTest: VulkanTester {
    explicit FramebufferVkTest();

    void construct();
    void constructMove();

    void wrap();
};

FramebufferVkTest::FramebufferVkTest() {
    addTests({&FramebufferVkTest::construct,
              &FramebufferVkTest::constructMove,

              &FramebufferVkTest::wrap});
}

void FramebufferVkTest::construct() {
    /* Using a depth attachment as well even though not strictly necessary to
       catch potential unexpected bugs */
    Image color{device(), ImageCreateInfo2D{ImageUsage::ColorAttachment,
        VK_FORMAT_R8G8B8A8_UNORM, {256, 256}, 1}, MemoryFlag::DeviceLocal};
    Image depth{device(), ImageCreateInfo2D{ImageUsage::DepthStencilAttachment,
        VK_FORMAT_D24_UNORM_S8_UINT, {256, 256}, 1}, MemoryFlag::DeviceLocal};
    ImageView colorView{device(), ImageViewCreateInfo2D{color}};
    ImageView depthView{device(), ImageViewCreateInfo2D{depth}};

    RenderPass renderPass{device(), RenderPassCreateInfo{}
        .setAttachments({
            color.format(),
            depth.format()
        })
        .addSubpass(SubpassDescription{}
            .setColorAttachments({0})
            .setDepthStencilAttachment(1)
        )
    };

    {
        Framebuffer framebuffer{device(), FramebufferCreateInfo{renderPass, {
            colorView,
            depthView
        }, {256, 256}}};
        CORRADE_VERIFY(framebuffer.handle());
        CORRADE_COMPARE(framebuffer.handleFlags(), HandleFlag::DestroyOnDestruction);
    }

    /* Shouldn't crash or anything */
    CORRADE_VERIFY(true);
}

void FramebufferVkTest::constructMove() {
    Image color{device(), ImageCreateInfo2D{ImageUsage::ColorAttachment,
        VK_FORMAT_R8G8B8A8_UNORM, {256, 256}, 1}, MemoryFlag::DeviceLocal};
    ImageView colorView{device(), ImageViewCreateInfo2D{color}};
    RenderPass renderPass{device(), RenderPassCreateInfo{}
        .setAttachments({color.format()})
        .addSubpass(SubpassDescription{}.setColorAttachments({0}))
    };

    Framebuffer a{device(), FramebufferCreateInfo{renderPass, {
        colorView
    }, {256, 256}}};
    VkFramebuffer handle = a.handle();

    Framebuffer b = std::move(a);
    CORRADE_VERIFY(!a.handle());
    CORRADE_COMPARE(b.handle(), handle);
    CORRADE_COMPARE(b.handleFlags(), HandleFlag::DestroyOnDestruction);

    Framebuffer c{NoCreate};
    c = std::move(b);
    CORRADE_VERIFY(!b.handle());
    CORRADE_COMPARE(b.handleFlags(), HandleFlags{});
    CORRADE_COMPARE(c.handle(), handle);
    CORRADE_COMPARE(c.handleFlags(), HandleFlag::DestroyOnDestruction);

    CORRADE_VERIFY(std::is_nothrow_move_constructible<Framebuffer>::value);
    CORRADE_VERIFY(std::is_nothrow_move_assignable<Framebuffer>::value);
}

void FramebufferVkTest::wrap() {
    Image color{device(), ImageCreateInfo2D{ImageUsage::ColorAttachment,
        VK_FORMAT_R8G8B8A8_UNORM, {256, 256}, 1}, MemoryFlag::DeviceLocal};
    ImageView colorView{device(), ImageViewCreateInfo2D{color}};
    RenderPass renderPass{device(), RenderPassCreateInfo{}
        .setAttachments({color.format()})
        .addSubpass(SubpassDescription{}.setColorAttachments({0}))
    };

    VkFramebuffer framebuffer{};
    CORRADE_COMPARE(Result(device()->CreateFramebuffer(device(),
        FramebufferCreateInfo{renderPass, {colorView}, {256, 256}},
        nullptr, &framebuffer)), Result::Success);

    auto wrapped = Framebuffer::wrap(device(), framebuffer, HandleFlag::DestroyOnDestruction);
    CORRADE_COMPARE(wrapped.handle(), framebuffer);

    /* Release the handle again, destroy by hand */
    CORRADE_COMPARE(wrapped.release(), framebuffer);
    CORRADE_VERIFY(!wrapped.handle());
    device()->DestroyFramebuffer(device(), framebuffer, nullptr);
}

}}}}

CORRADE_TEST_MAIN(Magnum::Vk::Test::FramebufferVkTest)
