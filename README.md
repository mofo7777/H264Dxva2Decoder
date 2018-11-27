# H264Dxva2Decoder

## Program

This program parses mp4 file (avcc/avc1 format only), then parses NAL Unit, decodes slices using IDirectXVideoDecoder, then display pictures using IDirectXVideoProcessor. This program just uses Microsoft Windows SDK for Windows 7, visual studio community 2017, Mediafoundation API and C++. You need at minimum Windows Seven, and a GPU ables to fully hardware decode h264 video format. This program does not implements all h264 features, see limitations below.

## limitations

### GPU
* only tested DXVA2_ModeH264_E GPU mode
* minimal GPU for NVIDIA cards -> Feature Set C (VP4) see https://en.wikipedia.org/wiki/Nvidia_PureVideo
* the GPU decoding is OK with NVIDIA GeForce 700 series
* With Intel HD Graphics 4000, seems to be OK too

### ATOM
* just uses needed atoms
* sample time is not used for display : use 1000 / frame rate - 4 (custom frame rate approximation)
* just uses the first video track found
* partial sync sample atom is not handled
* multiple stds atom are not handled

### NALU
* NaluLenghtSize == 1 is not handled (never encountered such mp4 file)
* Remove Emulation Prevention Byte is not done (don't see any problem right now)
* just handles NAL_UNIT_CODED_SLICE and NAL_UNIT_CODED_SLICE_IDR, ignore others
* normally, uses same variable name as h264 spec : good for comparing and implementing
* needs a better check of min/max values according to h264 spec
* pic_order_cnt_type == 1 is not handled (never encountered such mp4 file)
* frame_mbs_only_flag is not handled
* rbsp_trailing_bits is not used (don't see any problem right now)
* num_slice_groups_minus1 is not handled
* redundant_pic_cnt_present_flag is not handled
* pic_scaling_matrix_present_flag is not handled
* memory_management_control_operation is not used
* SP/SI slice type are not handle
* only handles chroma_format_idc == 1

### DECODING
* only used DXVA_Slice_H264_Short, not DXVA_Slice_H264_Long (GPU must handle it) see ConfigBitstreamRaw
* no interlacing
* video only (no audio processing)
* complete gpu acceleration only, no software fallback
* long term reference and list reordering are not handled (never encountered such mp4 file)

### DISPLAY
* SetVideoProcessStreamState should use values from mp4 file, here default for all
* just use D3DFMT_X8R8G8B8 output format and D3DFMT_NV12 input format
* resize window to video size without checking screen resolution. For big video, windows position can be bad, depend on your screen resolution
