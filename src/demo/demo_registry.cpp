#include "demo_registry.h"

namespace vk_demo {

void forceLinkDemos() {}

} // namespace vk_demo
namespace vk_demo {

extern void* _DemoTriangle_symbol;
void* _force_link_triangle = _DemoTriangle_symbol;

extern void* _DemoMultiPassBlend_symbol;
void* _force_link_multi_pass_blend = _DemoMultiPassBlend_symbol;


}