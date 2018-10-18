# H264Dxva2Decoder

## Program

This program parses mp4 file (avcc format only), then parses NAL Unit, decodes slices using IDirectXVideoDecoder, then display pictures using IDirectXVideoProcessor. This program just uses Microsoft Windows SDK for Windows 7, visual studio community 2017, Mediafoundation API and C++. You need at minimum Windows Seven, and a GPU ables to fully hardware decode h264 video format. This program does not implements all h264 features, see limitations below.

## limitations

### GPU
* DXVA2_ModeH264_E only, try use other when 4K
* minimal GPU -> Feature Set C (VP4) see https://en.wikipedia.org/wiki/Nvidia_PureVideo
* the GPU decoding is OK with NVIDIA GeForce 700 series
* With Intel HD Graphics 4000, there are artifacts. Need to investigate

### FILE
* CMFByteStream : has SeekHigh but not used when decoding, only parsing (SeekFile == SeekHigh ... need to remove one), see GetNextSample

### ATOM
* just use needed atoms
* sample time not used : use 1000 / frame rate - 4
* just use the first video track found
* sometimes ATOM_TYPE_AVCC is not the first atom in ATOM_TYPE_STSD, need to loop (can be others)

### NALU
* NaluLenghtSize == 1 not handle (never encountered such mp4 file)
* handle only case where SPS/PPS does not change
* Remove Emulation Prevention Byte is not done (don't see any problem right now)
* just handle NAL_UNIT_CODED_SLICE and NAL_UNIT_CODED_SLICE_IDR, ignore others
* normally, use same variable name as h264 spec : good for comparing and implementing
* need a better check of min/max values according to h264 spec
* pic_order_cnt_type == 1 is not handle (never encountered such mp4 file)
* frame_mbs_only_flag is not handle
* rbsp_trailing_bits is not used (don't see any problem right now)
* pic_order_present_flag is not handle
* num_slice_groups_minus1 is not handle
* redundant_pic_cnt_present_flag is not handle
* pic_scaling_matrix_present_flag is not handle
* memory_management_control_operation is not used
* SP/SI slice type are not handle

### DECODING
* only use DXVA_Slice_H264_Short not DXVA_Slice_H264_Long (GPU must handle it) see ConfigBitstreamRaw
* no interlacing
* video only (no audio processing)
* complete gpu acceleration only
* Quanta Matrix just use default matrix values
* long term reference and list reordering is not handle (never encountered such mp4 file)

### DISPLAY
* SetVideoProcessStreamState should use values from mp4 file, here default for all
* just use D3DFMT_X8R8G8B8 output format and D3DFMT_NV12 input format
* resize window to video size without checking screen resolution
