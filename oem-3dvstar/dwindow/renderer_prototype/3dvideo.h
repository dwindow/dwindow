// Nvidia Stereo signature

// Stereo Blitdefines
#define NVSTEREO_IMAGE_SIGNATURE 0x4433564e //NV3D


typedef struct _Nv_Stereo_Image_Header
{
unsigned int dwSignature;
unsigned int dwWidth;
unsigned int dwHeight;
unsigned int dwBPP;
unsigned int dwFlags;
} NVSTEREOIMAGEHEADER, *LPNVSTEREOIMAGEHEADER;


// ORedflags in the dwFlagsfielsof the _Nv_Stereo_Image_Headerstructure above
#define SIH_SIDE_BY_SIDE 0x00000000
#define SIH_SWAP_EYES 0x00000001
#define SIH_SCALE_TO_FIT 0x00000002