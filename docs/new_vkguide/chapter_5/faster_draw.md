---
layout: default
title: Faster Draw
parent: "New 5. GLTF loading"
nav_order: 10
---

# Improving Performance

When we made the draw loop on chapter 4, we did not try to skip vulkan calls if they are the same between RenderObjects. Lets improve that.

We need to keep track of what state we are binding, and only call it again if we have to as it changes with the draw. 

We are going to modify the draw() lambda seen in the last article, and give it state tracking. It will only call the vulkan functions if the parameters change.

```cpp
//defined outside of the draw function, this is the state we will try to skip
 MaterialPipeline* lastPipeline = nullptr;
 MaterialInstance* lastMaterial = nullptr;
 VkBuffer lastIndexBuffer = VK_NULL_HANDLE;

 auto draw = [&](const RenderObject& r) {
     if (r.material != lastMaterial) {
         lastMaterial = r.material;
         //rebind pipeline and descriptors if the material changed
         if (r.material->pipeline != lastPipeline) {

             lastPipeline = r.material->pipeline;
             vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, r.material->pipeline->pipeline);
             vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,r.material->pipeline->layout, 0, 1,
                 &globalDescriptor, 0, nullptr);

            VkViewport viewport = {};
            viewport.x = 0;
            viewport.y = 0;
            viewport.width = (float)_windowExtent.width;
            viewport.height = (float)_windowExtent.height;
            viewport.minDepth = 0.f;
            viewport.maxDepth = 1.f;

            vkCmdSetViewport(cmd, 0, 1, &viewport);

            VkRect2D scissor = {};
            scissor.offset.x = 0;
            scissor.offset.y = 0;
            scissor.extent.width = _windowExtent.width;
            scissor.extent.height = _windowExtent.height;

            vkCmdSetScissor(cmd, 0, 1, &scissor);
         }

         vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, r.material->pipeline->layout, 1, 1,
             &r.material->materialSet, 0, nullptr);
     }
    //rebind index buffer if needed
     if (r.indexBuffer != lastIndexBuffer) {
         lastIndexBuffer = r.indexBuffer;
         vkCmdBindIndexBuffer(cmd, r.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
     }
     // calculate final mesh matrix
     GPUDrawPushConstants push_constants;
     push_constants.worldMatrix = r.transform;
     push_constants.vertexBuffer = r.vertexBufferAddress;

     vkCmdPushConstants(cmd, r.material->pipeline->layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(GPUDrawPushConstants), &push_constants);

     vkCmdDrawIndexed(cmd, r.indexCount, 1, r.firstIndex, 0, 0);
 };
```

We store the last pipeline, the last material, and the last index buffer.
We begin by checking if the pipeline has changed, and if it did, we bind the pipeline again, and also re-bind the global descriptor set. We also have to call the SetViewport and SetScissor commands too.

Then, we bind the descriptor set it for material parameters and textures if the material instance changed. 
And last, the index buffer gets bound again if it changed.

We now should get a performance win, specially as we only have 2 pipelines, so a lot of those calls to VkCmdBindPipeline now dissapear. But lets improve it further.

We are going to sort the render objects by those parameters to minimize the number of calls. We will only do it this way for the opaque objects, as the transparent objects need a different type of sorting (depth sort) that we arent doing as we dont have the information about whats the center of the object.

To implement the sorting, we wont be sorting the draw array itself, as the objects are big. Instead, we are going to sort an array of indices to this draw array. This is a common technique in big engines.

At the beggining of the draw_geometry() function, add this

```cpp
    std::vector<uint32_t> opaque_draws;
    opaque_draws.reserve(drawCommands.OpaqueSurfaces.size());

    for (uint32_t i = 0; i < drawCommands.OpaqueSurfaces.size(); i++) {
        opaque_draws.push_back(i);
    }

    // sort the opaque surfaces by material and mesh
    std::sort(opaque_draws.begin(), opaque_draws.end(), [&](const auto& iA, const auto& iB) {
    const RenderObject& A = drawCommands.OpaqueSurfaces[iA];
    const RenderObject& B = drawCommands.OpaqueSurfaces[iB];
    if (A.material == B.material) {
        return A.indexBuffer < B.indexBuffer;
    } else {
        return A.material < B.material;
    }
    });
```

std::algorithms has a very handy sort function we can use to sort the opaque_draws vector. We give it a lambda that defines a `<` operator, and it sorts it efficiently for us. 

We will first index the draw array, and check if the material is the same, and if it is, sort by indexBuffer. But if its not, then we directly compare the material pointer. Another way of doing this is that we would calculate a sort key , and then our opaque_draws would be something like 20 bits draw index, and 44 bits for sort key/hash. That way would be faster than this as it can be sorted through faster methods.

Now, for the draws, we draw from the sorted array. Replace the draw loop with this one.

```cpp
for (auto& r : opaque_draws) {
    draw(drawCommands.OpaqueSurfaces[r]);
}
```


With this the renderer will minimize the number of descriptor set bindings, as it will go material by material. We still have the index buffer binding to deal with but thats faster to switch.

By doing this, the engine should now have significantly more performance. If you run it in release mode, you should be able to draw scenes with tens of thousands of meshes no problem. We arent doing frustrum culling, so the GPU gets a lot of wasted work which does affect its perf.

## Frustum Culling

There is another big issue the engine has at the moment. We are drawing the entire scene, even things that are outside of the view. As we have the draw list, we will filter it to check what objects are in view, and skip the ones that dont.

There are multiple ways of doing Frustum culling, but with the data and architecture we have, we will use oriented bounding boxes. We will calculate bounds for each GeoSurface, and then check if the bounds are in view.

Update the structures in vk_loader.h with the bounds.
```cpp
struct Bounds {
    glm::vec3 origin;
    float sphereRadius;
    glm::vec3 extents;
};

struct GeoSurface {
    uint32_t startIndex;
    uint32_t count;
    Bounds bounds;
	std::shared_ptr<GLTFMaterial> material;
};
```

Our bounds are a origin, extent (box size), and sphere radius. The sphere radius can be used in case we want to use other frustum culling algorithms and has other uses.

To calculate it, we must add it to the loader code. 

This code goes inside the loadGLTF function, at the end of the loop that loads the mesh data.

```cpp
//code that writes vertex buffers

//loop the vertices of this surface, find min/max bounds
glm::vec3 minpos = vertices[initial_vtx].position;
glm::vec3 maxpos = vertices[initial_vtx].position;
for (int i = initial_vtx; i < vertices.size(); i++) {
    minpos = glm::min(minpos, vertices[i].position);
    maxpos = glm::max(maxpos, vertices[i].position);
}
// calculate origin and extents from the min/max, use extent lenght for radius
newSurface.bounds.origin = (maxpos + minpos) / 2.f;
newSurface.bounds.extents = (maxpos - minpos) / 2.f;
newSurface.bounds.sphereRadius = glm::length(newSurface.bounds.extents);

newmesh->surfaces.push_back(newSurface);
```

Now we have the bounds on the GeoSurface, and just need to check for visibility on the RenderObject. Add this function to vk_engine.cpp, its a global function.


<!-- codegen from tag visfn on file E:\ProgrammingProjects\vulkan-guide-2\chapter-5/vk_engine.cpp --> 
```cpp
bool is_visible(const RenderObject& obj, const glm::mat4& viewproj) {
    std::array<glm::vec3, 8> corners {
        glm::vec3 { 1, 1, 1 },
        glm::vec3 { 1, 1, -1 },
        glm::vec3 { 1, -1, 1 },
        glm::vec3 { 1, -1, -1 },
        glm::vec3 { -1, 1, 1 },
        glm::vec3 { -1, 1, -1 },
        glm::vec3 { -1, -1, 1 },
        glm::vec3 { -1, -1, -1 },
    };

    glm::mat4 matrix = viewproj * obj.transform;

    glm::vec3 min = { 1.5, 1.5, 1.5 };
    glm::vec3 max = { -1.5, -1.5, -1.5 };

    for (int c = 0; c < 8; c++) {
        // project each corner into clip space
        glm::vec4 v = matrix * glm::vec4(obj.bounds.origin + (corners[c] * obj.bounds.extents), 1.f);

        // perspective correction
        v.x = v.x / v.w;
        v.y = v.y / v.w;
        v.z = v.z / v.w;

        min = glm::min(glm::vec3 { v.x, v.y, v.z }, min);
        max = glm::max(glm::vec3 { v.x, v.y, v.z }, max);
    }

    // check the clip space box is within the view
    if (min.z > 1.f || max.z < 0.f || min.x > 1.f || max.x < -1.f || min.y > 1.f || max.y < -1.f) {
        return false;
    } else {
        return true;
    }
}
```

This is just one of the multiple possible functions we could be using for frustum culling. The way this works is that we are transforming each of the 8 corners of the mesh-space bounding box into screenspace, using object matrix and view-projection matrix. For those, we find the screen-space box bounds, and we check if that box is inside the clip-space view. This way of calculating bounds is on the slow side compared to other formulas, and can have false-positives where it things objects are visible when they arent. All the functions have different tradeoffs, and this one was selected for code simplicity and parallels with the functions we are doing on the vertex shaders.
 
To use it, we change the loop we added to fill the opaque_draws array.

```cpp
for (int i = 0; i < drawCommands.OpaqueSurfaces.size(); i++) {
    if (is_visible(drawCommands.OpaqueSurfaces[i], sceneData.viewproj)) {
        opaque_draws.push_back(i);
    }
}
```

Now instead of adding `i` to it, we check for visibility. 

The renderer now will skip objects outside of the view. It should look the same as it did, but run faster and with less draws. If you get visual glitches, double-check the building of the bounding box for the `GeoSurface` and see if there is a typo in the is_visible function.

{% include comments.html term="Vkguide 2 Beta Comments" %}