#pragma once


class RenderPass {
public:
	virtual void RenderFrame() = 0;
	virtual void freeResources() = 0;
	virtual void recreateResources() = 0;
};

