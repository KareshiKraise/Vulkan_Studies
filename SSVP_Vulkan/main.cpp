#include <iostream>
#include "Renderer.h"
#include "ScreenQuadRenderPass.h" 

//#define STB_IMAGE_IMPLEMENTATION
//#include "stb_image.h" 


int main() {
    Vulkan_Renderer app;
    ScreenQuadRenderPass pass(app);
            
    app.mainLoop(&pass);
    //int texWidth, texHeight, texChannels;
    //stbi_uc* pixels = stbi_load("/models/textures/background.png", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    
    //if (!pixels)
    //{
    //    std::cout << "faled to open";
   // }

    std::cin.get();
    return 0;
}