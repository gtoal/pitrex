


int loadAndPlayRAW()
{
    TCHAR *FILE_NAME ="pitrexSample.raw";
    FRESULT rc_rd = FR_DISK_ERR;
    FIL file_object_rd;
    rc_rd = f_open(&file_object_rd, FILE_NAME, (unsigned char) FA_READ);

    if (rc_rd != FR_OK)
    {
      printf("Could not open file %s (%i) \r\n", FILE_NAME, rc_rd);
    }
    else
    {
    /*			
    FIL* fp, 	/* Pointer to the file object 
    void* buff,	/* Pointer to data buffer 
    UINT btr,	/* Number of unsigned chars to read 
    UINT* br	/* Pointer to number of unsigned chars read 
    */
      printf("Loading: %s \r\n", FILE_NAME);
      unsigned int fsize = 200*1000-1;
      rc_rd = f_read(&file_object_rd, ymBufferLoad, fsize, &fsize);
      if ( rc_rd!= FR_OK)
      {
	      printf("File not loaded (size got = %i)\r\n", fsize);
	      f_close(&file_object_rd);
		  return 0;
      }
      else
      {
	  f_close(&file_object_rd);
	  // file is loaded
	  printf("File loaded successfully (%i)!\r\n",fsize);
// sample rate is 15000
  	  v_playDirectSampleAll(ymBufferLoad, fsize, 15000);     
      }
    }
    return 1;
}
