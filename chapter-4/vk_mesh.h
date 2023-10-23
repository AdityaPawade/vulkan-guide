﻿#pragma once

#include <vk_types.h>
#include <vector>
#include <glm/vec4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec2.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/quaternion.hpp>

struct DrawContext;

struct Vertex {

	glm::vec3 position;
	float uv_x;
	glm::vec3 normal;
	float uv_y;
	glm::vec4 color;
}; 

struct Surface {

	size_t indexCount;
	AllocatedBuffer _indexBuffer;
	AllocatedBuffer _vertexBuffer;
	VkDescriptorSet bufferBinding;
};

class IRenderable {

	virtual void Draw(const glm::mat4& topMatrix, DrawContext& ctx) = 0;
};