#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <ctype.h>
#include <sys/syscalls.h>


#define MAX_BUFF_SIZE 64

int _v_scanf( int (*_read)(void*), void (*_push)(void*, unsigned char),  void* arg, const unsigned char *, va_list);

/**********************************************************/
/* sscanf functions                                       */
/**********************************************************/
struct _sscanf_struct
{
    int count;
	int buff_pos;
	unsigned char* buff;
	unsigned char const *str;
    int pos;
};

static void _sscanf_push(void *p, unsigned char c)
{
	struct _sscanf_struct *val = (struct _sscanf_struct*)p;
	if(val->buff_pos < MAX_BUFF_SIZE)
		val->buff[val->buff_pos++] = c;
}

static int _sscanf_read(void* p)
{
	struct _sscanf_struct *val = (struct _sscanf_struct*)p;
    int ch;
	
	if(val->buff_pos > 0)
	{
		return val->buff[--val->buff_pos];
	}

    if((ch = val->str[val->pos++]) == 0)
    {
        val->pos--;
        return 0;
    }
    val->count++;
	return ch;
}

int vsscanf(char const *str, char const *format, va_list arg_ptr)
{
    struct _sscanf_struct *val;
    
	val->buff = (unsigned char*)malloc(MAX_BUFF_SIZE*sizeof(unsigned char));
	val->buff_pos = 0;
	val->count = 0;
    val->pos = 0;
    val->str = str;
    return _v_scanf(_sscanf_read, _sscanf_push, &val, format, arg_ptr);
}

/**********************************************************/
/* fscanf functions                                       */
/**********************************************************/

struct _fscanf_struct
{
	int buff_pos;
	unsigned char* buff;
	int count;
	FILE* stream;
};

static void _fscanf_push(void *p, unsigned char c)
{
	struct _fscanf_struct *val = (struct _fscanf_struct*)p;
	if(val->buff_pos < MAX_BUFF_SIZE)
		val->buff[val->buff_pos++] = c;
}

static int _fscanf_read(void *p)
{
	struct _fscanf_struct *val = (struct _fscanf_struct*)p;
	FILE* stream;

	if(val->buff_pos > 0)
	{
		return val->buff[--val->buff_pos];
	}

	stream = val->stream;

    if (stream->rpos >= stream->buf_pos) 
    {
        int len = sys_read(stream->fd, stream->buf, -1, stream->buf_size);
    
        if (len==0) 
        {
            stream->flags |= _STDIO_EOF;
            return EOF;
        } 
        else if (len < 0) 
        {
            stream->flags |= _STDIO_ERROR;
            return EOF;
        }
        stream->rpos=0;
        stream->buf_pos=len;
    }
	val->count++;
    return stream->buf[stream->rpos++];
}

int vfscanf( FILE *stream, char const *format, va_list arg_ptr)
{
	struct _fscanf_struct *val = (struct _fscanf_struct*)malloc(sizeof(struct _fscanf_struct));

	val->buff = (unsigned char*)malloc(MAX_BUFF_SIZE*sizeof(unsigned char));
	val->buff_pos = 0;
	val->count = 0;
	val->stream = stream;
    return _v_scanf(_fscanf_read, _fscanf_push, val, format, arg_ptr);
}

/**********************************************************/
/* value size determination functions                     */
/**********************************************************/

typedef enum
{
	SHORT,
	INT,
	LONG,
	LONGLONG
} valSize;
/*
static valSize _getValSize(char c, int (*_read)(void*), void* arg)
{
	if(c == 'h')
	{
		return SHORT;
	}
	if(c == 'l')
	{
		return LONG;
	}
	if( c == 'L')
	{
		return LONGLONG;
	}
	return INT;
}
*/
/**********************************************************/
/* utility functions                                      */
/**********************************************************/
/*
static bool _isIntegralType(char c)
{
	return c == 'd' || c == 'i' || c == 'o' || c == 'u' || c == 'x'
		|| c == 'X' || c == 'p';

}

static bool _isFloatingType(char c)
{
	return c == 'E' || c == 'G'
		|| c == 'e' || c == 'f' || c == 'g';
}

static bool _isType(char c)
{
	return _isIntegralType(c) || _isFloatingType(c) || c == 'n' || c == 'c' || c == '%' || c == '[';
}
*/

static int _read_next_non_space( int (*_read)(void*), void* arg)
{
	int c = _read(arg);
	while(isspace(c))
	{
		c = _read(arg);
	}
	return c;
}

/*  The code for this function is mostly taken from strtoull */
//unsigned long long convertIntegralValue(int (*_read)(void*), void (*_push)(void*, unsigned char), void *arg, int base, short width);
static unsigned long long convertIntegralValue(int (*_read)(void*), void (*_push)(void*, unsigned char), void *arg, int base, short width)
{
	unsigned long long acc;
	unsigned char c;
	unsigned long long qbase, cutoff;
	int neg, any, cutlim;
	int length = 0;


	do {
		c = _read(arg);
	} while (isspace(c));

	if(width == 0)
	{
		return 0;
	}

	if (c == '-') {
		neg = 1;
		length++;
		c = _read(arg);
	} else {
		neg = 0;
		if (c == '+')
		{
			length++;
			c = _read(arg);
		}
	}

	if(width > 0 && length >= width)
	{
		return 0;
	}

	if (base == 0 || base == 16)
	{
		unsigned char c_next = _read(arg);
	    if(c == '0' && (c_next == 'x' || c_next == 'X'))
		{
			c = _read(arg);
			base = 16;
		}
		else
		{
			_push(arg, c_next);
		}
	}

	if(width > 0 && length >= width)
	{
		return 0;
	}	

	if (base == 0)
	{
		base = c == '0' ? 8 : 10;
	}

	qbase = (unsigned)base;
	cutoff = (unsigned long long)ULLONG_MAX / qbase;
	cutlim = (unsigned long long)ULLONG_MAX % qbase;


	for (acc = 0, any = 0;; c = _read(arg), length++) 
	{
		if(width > 0 && length > width)
		{
			goto finish;
		}	
		
		if (!isascii(c))
			break;
		if (isdigit(c))
			c -= '0';
		else if (isalpha(c))
			c -= isupper(c) ? 'A' - 10 : 'a' - 10;
		else
			break;
		if (c >= base)
			break;
		if (any < 0 || acc > cutoff || (acc == cutoff && c > cutlim))
			any = -1;
		else 
		{
			any = 1;
			acc *= qbase;
			acc += c;
		}
	}

finish:
	_push(arg, c);

	if (any < 0) 
	{
		acc = ULLONG_MAX;
	}
	else if (neg)
		acc = -acc;

	return (acc);
}


/**********************************************************/
/* the function                                           */
/**********************************************************/
int _v_scanf( int (*_read)(void*), void (*_push)(void*, unsigned char),  void* arg, const unsigned char *format, va_list arg_ptr)
{
	unsigned int fieldsRead = 0;
	unsigned char fch;

	while (*format)
	{
		fch = *format++;
    	if(isspace(fch))
    	{
			
		 	if(isspace(_read(arg)))
			{
				break;
			}
			return EOF;
		}
		else if(fch != '%')
		{
			if(fch == _read_next_non_space(_read, arg))
			{
				break;
			}
			return EOF;
		}
		else
		{
			bool suppressAssignment = false;
			short width = -1;
			valSize size = INT;

keeplooking:

			fch = *format++;

			switch(fch)
			{   
				unsigned short* us;
				short* s;
				unsigned int* ui;
				int* i;
				unsigned long* ul;
				long* l;
				unsigned long long* ull;
				long long* ll;
				case '%':
					if(_read_next_non_space(_read, arg) != '%')
					{
						return EOF;
					}
					break;
				case '*':
					suppressAssignment = true;
					fch = *format++;
					goto keeplooking;
				case '0':
				case '1':
				case '2':
				case '3':
				case '4':
				case '5':
				case '6':
				case '7':
				case '8':
				case '9':
					do
					{
						if(width == -1)
						{
							width = 0;
						}
						width = 10 * width + (unsigned int)(fch - (unsigned char)'0');
						fch = *format++;
					}
					while(isdigit(fch));
					goto keeplooking;

				case 'h':
					size = SHORT;
					goto keeplooking;
				case 'l':
					size = LONG;
					goto keeplooking;
				case 'L':
					size = LONGLONG;
					goto keeplooking;
				case 'u':
					fieldsRead++;
					unsigned long long temp = convertIntegralValue(_read, _push, arg, 10, width);
					switch(size)
					{
						case SHORT:
							us = (unsigned short*)va_arg(arg_ptr, unsigned short*);
							if(temp > USHRT_MAX)
							{
								*us = USHRT_MAX;
								break;
							}
							*us = (unsigned short)temp;	
						break;
						case INT:
							ui = (unsigned int*)va_arg(arg_ptr, unsigned int*);
							if(temp > UINT_MAX)
							{
								*ui = UINT_MAX;
								break;
							}
							*ui = (unsigned int)temp;	
						break;
						case LONG:
							ul = (unsigned long*)va_arg(arg_ptr, unsigned long*);
							if(temp > ULONG_MAX)
							{
								*ul = ULONG_MAX;
								break;
							}
							*ul = (unsigned long)temp;	
						break;
						default:
							ull = (unsigned long long*)va_arg(arg_ptr, unsigned long long*);
							if(temp > ULONG_LONG_MAX)
							{
								*ull = ULONG_LONG_MAX;
								break;
							}
							*ull = (unsigned long long)temp;
						break;
							
					}
					break;
				default:
					// not yet implemented
					return EOF;

			}
		}	
	}
	return fieldsRead;
}

