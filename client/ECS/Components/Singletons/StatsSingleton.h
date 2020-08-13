#pragma once
#include <NovusTypes.h>

struct EngineStatsSingleton
{
    struct Frame {
		f32 deltaTime;
		f32 simulationFrameTime;
		f32 renderFrameTime;
    };

	std::deque<Frame> frameStats;

	void AddTimings(f32 deltaTime, f32 simulationTime, f32 renderTime)
	{
		Frame newFrame;
		newFrame.deltaTime = deltaTime;
		newFrame.renderFrameTime = renderTime;
		newFrame.simulationFrameTime = simulationTime;

		//dont allow more than 120 frames stored
		if (frameStats.size() > 120)
		{
			frameStats.pop_back();
		}
		
		frameStats.push_front(newFrame);		
	}

	//averages a frame timing from the last {numFrames} frames
	Frame AverageFrame(int numFrames) {
		if (frameStats.size() > 0)
		{


			size_t count = (size_t)numFrames;
			if (numFrames > frameStats.size()) {
				count = frameStats.size();
			}

			Frame averaged = frameStats.front();

			for (uint32_t i = 1; i < count; i++) {
				Frame f = frameStats[i];

				averaged.deltaTime += f.deltaTime;
				averaged.renderFrameTime += f.renderFrameTime;
				averaged.simulationFrameTime += f.simulationFrameTime;
			}

			averaged.deltaTime /= count;
			averaged.renderFrameTime /= count;
			averaged.simulationFrameTime /= count;

			return averaged;
		}
		else {
			return Frame{ 0.f,0.f,0.f };
		}
	}
};