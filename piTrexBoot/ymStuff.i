
char ymBuffer[200*1000]; // max 200k ym buffer
char ymBufferLoad[200*1000]; // max 200k ym buffer


    //////////////////////////////////////
    ///////// YM_CONV.C - START //////////
    //////////////////////////////////////
    int vbl_len;
    int convert_ym3(unsigned char *out_buf, unsigned char *in_buf, int len)
    {
        int pos=4;
        int todo=((len-4)/14);
        vbl_len=todo;
        int out_counter = 0;
//        tl.printMessageSU(tab("VBL found", DATA_TAB)+todo+"");
//        song_name = file_name;

        while( todo != 0)
        {
            int i;
            for (i=0;i<14;i++)
            {
                unsigned char *outer = out_buf+i;
                unsigned char poker = in_buf[pos+(i*((len-4)/14))];

                if ((i==1)||(i==3)||(i==5))
                    poker &= 1+2+4+8;

                if ((i==6) )
                    poker &= 1+2+4+8+16;

                if (i==7)
                    poker &= 1+2+4+8+16+32;

                if (out_counter<len)
                    *(outer+out_counter*14) = poker;
            }
            out_counter++;
            pos++;
            todo--;
        }
        return vbl_len;
    }
    int convert_ym2(unsigned char *out_buf, unsigned char *in_buf, int len)
    {
        return convert_ym3(out_buf, in_buf, len);
    }

    int convert_ym3b(unsigned char *out_buf, unsigned char *in_buf, int len)
    {
         return convert_ym3(out_buf, in_buf, len);
    }

    int convert_ym4(unsigned char *out_buf, unsigned char *in_buf, int len)
    {
        int c1;
        int c2;
        int c3;
        int c4;
        int pos=12;

        c1 = in_buf[pos++]&0xff;
        c2 = in_buf[pos++]&0xff;
        c3 = in_buf[pos++]&0xff;
        c4 = in_buf[pos++]&0xff;
        int todo=c1*256*256*256+c2*256*256+c3*256+c4;
        vbl_len=todo;
        int out_counter = 0;
//        tl.printMessageSU(tab("VBL found", DATA_TAB)+todo+"");
        int interleave_length=todo;

        c1 = in_buf[pos++]&0xff;
        c2 = in_buf[pos++]&0xff;
        c3 = in_buf[pos++]&0xff;
        c4 = in_buf[pos++]&0xff;
        int attribut=c1*256*256*256+c2*256*256+c3*256+c4;
//        tl.printMessageSU(tab("Attributs found", DATA_TAB)+getLongBinaryString(attribut)+"");

        c1 = in_buf[pos++]&0xff;
        c2 = in_buf[pos++]&0xff;
        c3 = in_buf[pos++]&0xff;
        c4 = in_buf[pos++]&0xff;
        int samples=c1*256*256*256+c2*256*256+c3*256+c4;
//        if (samples!=0)
//            tl.printMessageSU(tab("Samples found", DATA_TAB)+samples+" (not converted!)");

        c1 = in_buf[pos++]&0xff;
        c2 = in_buf[pos++]&0xff;
        c3 = in_buf[pos++]&0xff;
        c4 = in_buf[pos++]&0xff;
        int loopStart = (c1*256*256*256+c2*256*256+c3*256+c4);
//        tl.printMessageSU(tab("Frame loop start", DATA_TAB)+loopStart+"");

        // skip samples!
        while (samples!=0)
        {
            long sample_length;
            c1 = in_buf[pos++]&0xff;
            c2 = in_buf[pos++]&0xff;
            c3 = in_buf[pos++]&0xff;
            c4 = in_buf[pos++]&0xff;
            sample_length=c1*256*256*256+c2*256*256+c3*256+c4;
            pos+=sample_length;
            samples--;
        }
//        song_name ="";
        char cc=0;
        do
        {
            cc = (char)in_buf[pos++];
//            if (cc != 0) song_name += cc;
        } while (cc!=0);
//        tl.printMessageSU(tab("Name of song", DATA_TAB)+song_name+"");

//        author ="";
        do
        {
            cc = (char)in_buf[pos++];
//            if (cc != 0) author += cc;
        } while (cc!=0);
//        tl.printMessageSU(tab("Name of author", DATA_TAB)+author+"");

//        comment ="";
        do
        {
            cc = (char)in_buf[pos++];
//            if (cc != 0) comment += cc;
        } while (cc!=0);
//        tl.printMessageSU(tab("Comment", DATA_TAB)+comment+"");

        int interleave=((attribut&1) == 1);
        if (interleave)
        {
            printf("Using interleave format!\r\n");
            while( todo !=0)
            {
               int i;
               for (i=0;i<14;i++)
               {
                   unsigned char poker = in_buf[pos+i*interleave_length];
					unsigned char *outer = out_buf+i;
                   if ((i==1)||(i==3)||(i==5))
                       poker &= 1+2+4+8;

                   if ((i==6) )
                       poker &= 1+2+4+8+16;

                   if (i==7)
                       poker &= 1+2+4+8+16+32;
                    if (out_counter<len)
						*(outer+out_counter*14) = poker;
               }
               out_counter++;
               pos++;
               todo--;
           }
        }
        else
        {
            printf("Using non interleave format!\r\n");
            while( todo !=0)
            {
               int i;
               for (i=0;i<16;i++)
               {
                   unsigned char poker = in_buf[pos++];
   				   unsigned char *outer = out_buf+i;
                   if ((i==1)||(i==3)||(i==5))
                       poker &= 1+2+4+8;

                   if ((i==6) )
                       poker &= 1+2+4+8+16;

                   if (i==7)
                       poker &= 1+2+4+8+16+32;
				if (i<15)
					if (out_counter<len)
						*(outer+out_counter*14) = poker;
               }
               out_counter++;
               todo--;
           }
        }
        return vbl_len;
    }

    int convert_ym5(unsigned char *out_buf, unsigned char *in_buf, int len)
    {
	printf("in buf at = %X!\r\n",in_buf);
	printf("out buf at = %X!\r\n",out_buf);

	int c1;
        int c2;
        int c3;
        int c4;
        int pos=12;
        int out_counter = 0;

        c1 = in_buf[pos++]&0xff;
        c2 = in_buf[pos++]&0xff;
        c3 = in_buf[pos++]&0xff;
        c4 = in_buf[pos++]&0xff;
        int todo=c1*256*256*256+c2*256*256+c3*256+c4;
        vbl_len=todo;
        printf("VBL found: %i\r\n",vbl_len);
        int interleave_length=todo;

        c1 = in_buf[pos++]&0xff;
        c2 = in_buf[pos++]&0xff;
        c3 = in_buf[pos++]&0xff;
        c4 = in_buf[pos++]&0xff;
        int attribut=c1*256*256*256+c2*256*256+c3*256+c4;
//        tl.printMessageSU(tab("Attributes found", DATA_TAB)+getLongBinaryString(attribut)+"");

        c3 = in_buf[pos++]&0xff;
        c4 = in_buf[pos++]&0xff;
        int samples=c3*256+c4;
//        if (samples != 0)
//            tl.printMessageSU(tab("Samples found", DATA_TAB)+samples+" (not converted!)");

        c1 = in_buf[pos++]&0xff;
        c2 = in_buf[pos++]&0xff;
        c3 = in_buf[pos++]&0xff;
        c4 = in_buf[pos++]&0xff;
        int externalFrequency = (c1*256*256*256+c2*256*256+c3*256+c4);
//        tl.printMessageSU(tab("YM2149 External frequency in Hz", DATA_TAB)+externalFrequency+"");

        c3 = in_buf[pos++]&0xff;
        c4 = in_buf[pos++]&0xff;
        
        int playerFrequency = (c3*256+c4);
//        tl.printMessageSU(tab("Player frequency in Hz", DATA_TAB)+playerFrequency+"");

        c1 = in_buf[pos++]&0xff;
        c2 = in_buf[pos++]&0xff;
        c3 = in_buf[pos++]&0xff;
        c4 = in_buf[pos++]&0xff;
        int loopStart = (c1*256*256*256+c2*256*256+c3*256+c4);
//        tl.printMessageSU(tab("Vbl number to loop the song", DATA_TAB)+loopStart+" (unused)");

        c3 = in_buf[pos++]&0xff;
        c4 = in_buf[pos++]&0xff;
        int futureDataLength = (c3*256+c4);
//        tl.printMessageSU(tab("Size (in unsigned chars) of future data", DATA_TAB)+futureDataLength+"");

        // skip samples!
        while (samples != 0)
        {
            long sample_length;
            c1 = in_buf[pos++]&0xff;
            c2 = in_buf[pos++]&0xff;
            c3 = in_buf[pos++]&0xff;
            c4 = in_buf[pos++]&0xff;
            sample_length=c1*256*256*256+c2*256*256+c3*256+c4;
            pos+=sample_length;
            samples--;
        }
//        song_name ="";
        char cc=0;
        do
        {
            cc = (char)in_buf[pos++];
//            if (cc != 0) song_name += cc;
        } while (cc!=0);
//        tl.printMessageSU(tab("Name of song", DATA_TAB)+song_name+"");

//        author ="";
        do
        {
            cc = (char)in_buf[pos++];
//            if (cc != 0) author += cc;
        } while (cc!=0);
//        tl.printMessageSU(tab("Name of author", DATA_TAB)+author+"");

//        comment ="";
        do
        {
            cc = (char)in_buf[pos++];
//            if (cc != 0) comment += cc;
        } while (cc!=0);
//        tl.printMessageSU(tab("Comment", DATA_TAB)+comment+"");

        int interleave=((attribut&1) == 1);
        if (interleave)
        {
            printf("Using interleave format!\r\n");
            while( todo !=0)
            {
               int i;
               for (i=0;i<14;i++)
               {
                   unsigned char poker = in_buf[pos+i*interleave_length];
 		   unsigned char *outer = out_buf+i;
                   if ((i==1)||(i==3)||(i==5))
                       poker &= 1+2+4+8;

                   if ((i==6) )
                       poker &= 1+2+4+8+16;

                   if (i==7)
                       poker &= 1+2+4+8+16+32;
                   if (out_counter<len)
		   {
//		     printf("%i = %i ",(outer+out_counter*14), poker);
		      *(outer+out_counter*14) = poker;
		   }
               }
               out_counter++;
               pos++;
               todo--;
           }
        }
        else
        {
            printf("Using non interleave format!\r\n");
            while( todo !=0)
            {
               int i;
               for (i=0;i<16;i++)
               {
                   unsigned char poker = in_buf[pos++];
   				   unsigned char *outer = out_buf+i;
                   if ((i==1)||(i==3)||(i==5))
                       poker &= 1+2+4+8;

                   if ((i==6) )
                       poker &= 1+2+4+8+16;

                   if (i==7)
                       poker &= 1+2+4+8+16+32;
				if (i<15)
					if (out_counter<len)
						*(outer+out_counter*14) = poker;
               }
               out_counter++;
               todo--;
           }
        }
        return vbl_len;
    }

    int initym(int fsize)
    {
        vbl_len = 0;
        unsigned char *buf = ymBufferLoad;
        unsigned char *out_buf = ymBuffer;
        int len=fsize;        
        char format[5];
        format[0] = (char)buf[0];
        format[1] = (char)buf[1];
        format[2] = (char)buf[2];
        format[3] = (char)buf[3];
        format[4] = 0;
        printf("Length of file: %i\r\n",len);
        printf("Format: %s\r\n",format);

        if (format[2] == (char)'2')
        {
	    printf("YM2! format \r\n");
            vbl_len=convert_ym2(out_buf, buf,len);
        }
        else if (format[2] == (char)'3')
        {
			if ((format[3] == 'b')||(format[3] == 'B'))
			{
				printf("YM3b! format \r\n");
				vbl_len=convert_ym3b(out_buf,buf,len);
			}
			else
			{
				printf("YM3! format \r\n");
				vbl_len=convert_ym3(out_buf,buf,len);
			}
        }
        else if (format[2] == (char)'4')
        {
	    printf("YM4! format \r\n");
            vbl_len=convert_ym4(out_buf,buf,len);
        }
        else if (format[2] == (char)'5')
        {
	    printf("YM5! format \r\n");
            vbl_len=convert_ym5(out_buf,buf,len);
        }
        else if (format[2] == (char)'6')
        {
	    printf("YM6! format \r\n");
	    vbl_len=convert_ym5(out_buf,buf,len);
        }
        else
        {
            printf("Unkown or unsupported format!\r\n");
            return 0;
        }

        if (vbl_len == 0)
        {
            return 0;
        }
	return vbl_len;
    }


int loadYM()
{
    TCHAR *FILE_NAME ="menu.ym";
    FRESULT rc_rd = FR_DISK_ERR;
    FIL file_object_rd;
    rc_rd = f_open(&file_object_rd, FILE_NAME, (unsigned char) FA_READ);

    if (rc_rd != FR_OK)
    {
      printf("Could not open file %s (%i) \r\n", FILE_NAME, rc_rd);
      return 0;
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
	      printf("File not loaded (size got = %i) (error: %s)\r\n", fsize, getErrorText(rc_rd));
	      f_close(&file_object_rd);
		  return 0;
      }
      else
      {
	      f_close(&file_object_rd);
	      // file is loaded
	      printf("File loaded successfully (%i)!\r\n",fsize);

	int length = initym(fsize);
	v_initYM(ymBuffer, length, 1);      

      }
    }
	return 1;
}

