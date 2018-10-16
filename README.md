# H264Dxva2Decoder

## In progress

## limitations

* GPU
** GPU : DXVA2_ModeH264_E only, try use other when 4K
** GPU : minimal -> Feature Set C (VP4) see https://en.wikipedia.org/wiki/Nvidia_PureVideo

* FILE : CMFByteStream : has SeekHigh but not used when decoding, only parsing (SeekFile == SeekHigh ... need to remove one), see GetNextSample

* ATOM : just use needed atoms
* ATOM : sample time not used : use 1000 / frame rate - 4
* ATOM : Just use the first video track found
* ATOM : sometimes ATOM_TYPE_AVCC is not the first atom in ATOM_TYPE_STSD, need to loop (can be others)

* NALU : NaluLenghtSize == 1 not handle
* NALU : handle only case where SPS/PPS does not change
* NALU : Remove Emulation Prevention Byte is not done (don't see any problem right now)
* NALU : just handle NAL_UNIT_CODED_SLICE and NAL_UNIT_CODED_SLICE_IDR, ignore others
* NALU : normally, use same variable name as h264 spec : good for comparing and implementing
* NALU : need a better check of min/max values according to h264 spec
* NALU : pic_order_cnt_type == 1 is not handle
* NALU : frame_mbs_only_flag is not handle
* NALU : rbsp_trailing_bits is not used (don't see any problem right now)
* NALU : pic_order_present_flag is not handle
* NALU : num_slice_groups_minus1 is not handle
* NALU : redundant_pic_cnt_present_flag is not handle
* NALU : pic_scaling_matrix_present_flag is not handle
* NALU : memory_management_control_operation is not used

* DECODING : only use DXVA_Slice_H264_Short not DXVA_Slice_H264_Long (GPU must handle it) see ConfigBitstreamRaw
* DECODING : no interlacing
* DECODING : video only (no audio processing)
* DECODING : complete gpu acceleration only
* DECODING : Quanta Matrix just use default matrix values
* DECODING : long term reference and list reordering is not handle (never encountered such mp4 file)

* DISPLAY : SetVideoProcessStreamState should use values from mp4 file, here default for all
* DISPLAY : just use D3DFMT_X8R8G8B8 output format and D3DFMT_NV12 input format
* DISPLAY : resize window to video size without checking screen resolution
