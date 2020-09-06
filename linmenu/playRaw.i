


int loadAndPlayRAW()
{
	size_t rc_rd;
	char FILE_NAME[] = SOUNDPATH "pitrexSample.raw";
    FILE *file_object_rd;
    file_object_rd = fopen(FILE_NAME, "r");

    if (file_object_rd == NULL)
    {
      printf("Could not open file %s\r\n", FILE_NAME);
    }
    else
    {
      printf("Loading: %s \r\n", FILE_NAME);
      unsigned int fsize = 200*1000-1;
	  rc_rd = fread(ymBufferLoad, fsize, 1, file_object_rd);
/*
      if ( rc_rd != 1 )
      {
	      printf("File not loaded - read error or unexpected end of file\r\n");
	      fclose(file_object_rd);
		  return 0;
      }
      else
      {
*/
	  fclose(file_object_rd);
	  // file is loaded
	  printf("File loaded successfully (%i)!\r\n",fsize);
// sample rate is 15000
  	  v_playDirectSampleAll(ymBufferLoad, fsize/3, 15000);     
//      }
    }
    return 1;
}
