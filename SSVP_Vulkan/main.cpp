#include <iostream>
#include "Renderer.h"
#include "ScreenQuadRenderPass.h" 



int main() {
    Vulkan_Renderer app;
    ScreenQuadRenderPass pass(app);
            
    app.mainLoop(&pass);
    
    return 0;
}