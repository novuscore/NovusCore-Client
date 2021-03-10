struct Constants
{
	uint sourceByteOffset;
	uint targetByteOffset;
	uint threadGroupSize;
};

[[vk::binding(0, PER_DRAW)]] ByteAddressBuffer _source;
[[vk::binding(1, PER_DRAW)]] RWByteAddressBuffer _target;

[[vk::push_constant]] Constants _constants;

[numthreads(1, 1, 1)]
void main()
{
	const uint threadCount = _source.Load(_constants.sourceByteOffset);
	const uint threadGroupCount = (threadCount + _constants.threadGroupSize - 1) / _constants.threadGroupSize;
	_target.Store3(_constants.targetByteOffset, uint3(threadGroupCount, 1, 1));
}