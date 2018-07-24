#ifndef _IOCTL_IVS_H_

	#define _IOCTL_IVS_H_
     
    #define IVS_VER           	0x00200100
    //                            HIIFFFBB
    //                            H 		huge change
    //                             II 		interface change
    //                               FFF 	functional modified or bug fixed
    //                                  BB 	branch for customer request

	#define IVS_INT_VER       	0x13031100 	//	added for internal release only
    //                            YYMMDDSS 
    //                            YY 		year
    //                              MM 		month
    //                                DD 	date
    //                                  SS 	sequence number

	#define IVS_VER_MAJOR     	((IVS_VER>>28)&0x0f)   // huge change
    #define IVS_VER_MINOR     	((IVS_VER>>20)&0xff)   // inerface change
    #define IVS_VER_MINOR2    	((IVS_VER>>8)&0xfff)   // functional modified or bug fixed
    #define IVS_VER_BRANCH    	((IVS_VER>>0)&0x0ff)        // branch for customer request
	
    #define IVS_HSI_CONVERSION     	0x4170
    #define IVS_DE_INTERLEAVE_YC    0x4172
    #define IVS_INTEGRAL_IMAGE    	0x4177
    #define IVS_SAD                 0x4178
	#define IVS_INIT                0x4179    
	
    #define IVS_DEV  				"/dev/ivs"

	#define MAX_IVS_NUM    			32
       
    typedef struct ivs_param_t{
	
		int img_width  ;
		int img_height ;
	
		int operation ;
		int blk_size  ;
		int swap	   ;	
		int endian    ;

	} ivs_param_t;

	typedef struct ivs_info_t{
	
		int fd;
		unsigned char *mmap_addr;
		
	} ivs_info_t;
#endif
