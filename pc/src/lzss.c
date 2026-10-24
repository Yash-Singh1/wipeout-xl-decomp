#define INDEX_BIT_COUNT 13
#define LENGTH_BIT_COUNT 4
#define WINDOW_SIZE (1 << INDEX_BIT_COUNT)
#define BREAK_EVEN ((1 + INDEX_BIT_COUNT + LENGTH_BIT_COUNT) / 9)
#define END_OF_STREAM 0
#define MOD_WINDOW(a) ((a) & (WINDOW_SIZE - 1))

#ifdef NETPLAY
/* NET build: the frame is laid out from these names (MSVC /Od), the 8 KB window is the last local. */
void ExpandData( unsigned char *inData, unsigned char *outData )
{
    int outputCount;
    unsigned long decodedBits;
    unsigned char bitMask;
    short inputFlags;
    unsigned char windowData[ WINDOW_SIZE ];
    short i;
    unsigned long bitValue;
    short currentIndex;
    short matchBase;
    short matchLen;
    short isChar;
    unsigned char decodedCh;

    outputCount = 0;
    inputFlags = 0;
    bitMask = 0x80;

    currentIndex = 1;
    for ( ; ; )
    {

        if ( bitMask == 0x80 )
            inputFlags = (short)*inData++;

        isChar = inputFlags & bitMask;
        bitMask >>= 1;
        if ( bitMask == 0 )
            bitMask = 0x80;

        if ( isChar )
        {
            bitValue = 1L << ( 8 - 1 );
            decodedBits = 0;
            while ( bitValue != 0)
            {
                if ( bitMask == 0x80 )
                    inputFlags = (short)*inData++;

                if ( inputFlags & bitMask )
                    decodedBits |= bitValue;
                bitValue >>= 1;
                bitMask >>= 1;

                if ( bitMask == 0 )
                    bitMask = 0x80;
            }
            decodedCh = (unsigned char) decodedBits;
            *outData++ = decodedCh;
            outputCount++;
            windowData[ currentIndex ] = decodedCh;
            currentIndex = MOD_WINDOW( currentIndex + 1 );
        }
        else
        {
            bitValue = 1L << ( INDEX_BIT_COUNT - 1 );
            decodedBits = 0;
            while ( bitValue != 0)
            {
                if ( bitMask == 0x80 )
                    inputFlags = (short)*inData++;

                if ( inputFlags & bitMask )
                    decodedBits |= bitValue;
                bitValue >>= 1;
                bitMask >>= 1;

                if ( bitMask == 0 )
                    bitMask = 0x80;
            }
            matchBase = (short) decodedBits;

            if ( matchBase == END_OF_STREAM )
                    break;

            bitValue = 1L << ( LENGTH_BIT_COUNT - 1 );
            decodedBits = 0;
            while ( bitValue != 0)
            {
                if ( bitMask == 0x80 )
                    inputFlags = (short)*inData++;

                if ( inputFlags & bitMask )
                    decodedBits |= bitValue;
                bitValue >>= 1;
                bitMask >>= 1;

                if ( bitMask == 0 )
                    bitMask = 0x80;
            }
            matchLen = (short) decodedBits;

            matchLen += BREAK_EVEN;

            for ( i = 0 ; i <= matchLen ; i++ )
            {
                decodedCh = windowData[ MOD_WINDOW( matchBase + i ) ];
                *outData++ = decodedCh;
                outputCount++;
                windowData[ currentIndex ] = decodedCh;
                currentIndex = MOD_WINDOW( currentIndex + 1 );
            }
        }
    }
}
#else
/* Expands an LZSS-compressed stream into the output buffer. */
void ExpandData( unsigned char	*inData, unsigned char	*outData )
{
	short i;
	short current_position;
	unsigned char decodedByte;
	short match_length;
	short match_position;
	unsigned long mask;
	unsigned long decodedBits;
	unsigned char inputBitMask;
	short inputByte;
	short	isLiteral;
	int outputCount;
	unsigned char window[ WINDOW_SIZE ];

	outputCount = 0;
	inputByte = 0;
	inputBitMask = 0x80;

	current_position = 1;
	for ( ; ; )
	{

		if ( inputBitMask == 0x80 )
			inputByte =  (short)*inData++;

		isLiteral = inputByte & inputBitMask;
		inputBitMask >>= 1;
		if ( inputBitMask == 0 )
			inputBitMask = 0x80;

		if ( isLiteral )
		{
			mask = 1L << ( 8 - 1 );
			decodedBits = 0;
			while ( mask != 0)
			{
				if ( inputBitMask == 0x80 )
					inputByte =  (short)*inData++;
		
				if ( inputByte & inputBitMask )
					decodedBits |= mask;
				mask >>= 1;
				inputBitMask >>= 1;

				if ( inputBitMask == 0 )
					inputBitMask = 0x80;
			}
			decodedByte = (unsigned char) decodedBits;
			*outData++ = decodedByte;
			outputCount++;
			window[ current_position ] = decodedByte;
			current_position = MOD_WINDOW( current_position + 1 );
		}
		else
		{
			mask = 1L << ( INDEX_BIT_COUNT - 1 );
			decodedBits = 0;
			while ( mask != 0)
			{
				if ( inputBitMask == 0x80 )
					inputByte =  (short)*inData++;
		
				if ( inputByte & inputBitMask )
					decodedBits |= mask;
				mask >>= 1;
				inputBitMask >>= 1;

				if ( inputBitMask == 0 )
					inputBitMask = 0x80;
			}
			match_position = (short) decodedBits;

			if ( match_position == END_OF_STREAM )
					break;

			mask = 1L << ( LENGTH_BIT_COUNT - 1 );
			decodedBits = 0;
			while ( mask != 0)
			{
				if ( inputBitMask == 0x80 )
					inputByte =  (short)*inData++;
		
				if ( inputByte & inputBitMask )
					decodedBits |= mask;
				mask >>= 1;
				inputBitMask >>= 1;

				if ( inputBitMask == 0 )
					inputBitMask = 0x80;
			}
			match_length = (short) decodedBits;

			match_length += BREAK_EVEN;

			for ( i = 0 ; i <= match_length ; i++ )
			{
				decodedByte = window[ MOD_WINDOW( match_position + i ) ];
				*outData++ = decodedByte;
				outputCount++;
				window[ current_position ] = decodedByte;
				current_position = MOD_WINDOW( current_position + 1 );
			}
		}
	}
}
#endif
