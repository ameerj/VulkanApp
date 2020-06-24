#pragma once

// Indices of Queue families (if they exist)

struct QueueFamilyIndices {
	int graphics_family = -1;


	bool isValid() {
		return graphics_family >= 0;
	}
};
