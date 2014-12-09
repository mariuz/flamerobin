/*
------------------------------------------------------------------------------
The original code is taken from:
http://www.burtleburtle.net/bob/c/randport.c
http://www.burtleburtle.net/bob/c/standard.h
http://www.burtleburtle.net/bob/c/rand.h

Those files contain the following copyright notice:
By Bob Jenkins.  My random number generator, ISAAC.  Public Domain

It was modified 06.Apr 2006. by Milan Babuskov:
- removed unused functions and defines
- rewrote all defines to be regular functions
- wrapped everything in C++ class named Isaac

These modifications are also put in public domain.

Usage:

Isaac isc(seed);
isc.getCipher(password_to_encrypt);

"seed" can be up to 256 characters long. It can contain multibyte characters
whose size should not be bigger than size of unsigned long int.

"password_to_encrypt" can be up to 32 characters long. Same conditions apply.
------------------------------------------------------------------------------
*/

#ifndef FR_ISAAC_H
#define FR_ISAAC_H

class Isaac
{
private:
    typedef unsigned long int ub4;   /* unsigned 4-byte quantities */
    struct randctx
    {
        ub4 randcnt;
        ub4 randrsl[256];
        ub4 randmem[256];
        ub4 randa;
        ub4 randb;
        ub4 randc;
    };
    typedef  struct randctx  randctx;
    randctx ctxM;

    void rngstep(ub4 mix,ub4& a,ub4& b,ub4*& mm,ub4*& m,ub4*& m2,ub4*& r,ub4& x, ub4& y)
    {
        x = *m;
        a = ((a^(mix)) + *(m2++)) & 0xffffffff;
        *(m++) = y = (((mm)[(x>>2)&(255)]) + a + b) & 0xffffffff;
        *(r++) = b = (((mm)[(y>>10)&(255)]) + x) & 0xffffffff;
    }

    void isaac(randctx* ctx)
    {
       ub4 a,b,x,y,*m,*mm,*m2,*r,*mend;
       mm=ctx->randmem; r=ctx->randrsl;
       a = ctx->randa; b = (ctx->randb + (++ctx->randc)) & 0xffffffff;
       for (m = mm, mend = m2 = m+128; m<mend; )
       {
            rngstep( a<<13, a, b, mm, m, m2, r, x, y);
            rngstep( a>>6 , a, b, mm, m, m2, r, x, y);
            rngstep( a<<2 , a, b, mm, m, m2, r, x, y);
            rngstep( a>>16, a, b, mm, m, m2, r, x, y);
       }
       for (m2 = mm; m2<mend; )
       {
            rngstep( a<<13, a, b, mm, m, m2, r, x, y);
            rngstep( a>>6 , a, b, mm, m, m2, r, x, y);
            rngstep( a<<2 , a, b, mm, m, m2, r, x, y);
            rngstep( a>>16, a, b, mm, m, m2, r, x, y);
       }
       ctx->randb = b; ctx->randa = a;
    }

    void mix(ub4& a,ub4& b,ub4& c,ub4& d,ub4& e,ub4& f,ub4& g,ub4& h)
    {
        a^=b<<11; d+=a; b+=c;
        b^=c>>2;  e+=b; c+=d;
        c^=d<<8;  f+=c; d+=e;
        d^=e>>16; g+=d; e+=f;
        e^=f<<10; h+=e; f+=g;
        f^=g>>4;  a+=f; g+=h;
        g^=h<<8;  b+=g; h+=a;
        h^=a>>9;  c+=h; a+=b;
    }

    /* use the contents of randrsl[] to initialize mm[]. */
    void randinit(randctx* ctx)
    {
        ub4 a,b,c,d,e,f,g,h;
        ub4 *m,*r;
        ctx->randa = ctx->randb = ctx->randc = 0;
        m=ctx->randmem;
        r=ctx->randrsl;
        a=b=c=d=e=f=g=h=0x9e3779b9;  /* the golden ratio */
        for (int i=0; i<4; ++i)          /* scramble it */
            mix(a,b,c,d,e,f,g,h);

        for (int i=0; i<256; i+=8)
        {
            a+=r[i  ]; b+=r[i+1]; c+=r[i+2]; d+=r[i+3];
            e+=r[i+4]; f+=r[i+5]; g+=r[i+6]; h+=r[i+7];
            mix(a,b,c,d,e,f,g,h);
            m[i  ]=a; m[i+1]=b; m[i+2]=c; m[i+3]=d;
            m[i+4]=e; m[i+5]=f; m[i+6]=g; m[i+7]=h;
        }
        for (int i=0; i<256; i+=8)
        {
            a+=m[i  ]; b+=m[i+1]; c+=m[i+2]; d+=m[i+3];
            e+=m[i+4]; f+=m[i+5]; g+=m[i+6]; h+=m[i+7];
            mix(a,b,c,d,e,f,g,h);
            m[i  ]=a; m[i+1]=b; m[i+2]=c; m[i+3]=d;
            m[i+4]=e; m[i+5]=f; m[i+6]=g; m[i+7]=h;
        }
        isaac(ctx);            /* fill in the first set of results */
        ctx->randcnt=256;  /* prepare to use the first set of results */
    }

public:
    Isaac(const wxString& masterpw)
    {
        for (int i=0; i<256; ++i)
            ctxM.randrsl[i]=(ub4)0;
        for (int i=0; i<256 && masterpw[i]; ++i)
            ctxM.randrsl[i] = (wxChar)masterpw[i];
        randinit(&ctxM);
        isaac(&ctxM);
    }

    wxString getCipher(const wxString& pwd)
    {
        wxString result;
        ub4 pw[33];
        for (int i=0; i<33; ++i)
            pw[i] = 0;
        for (int i=0; i < (int) pwd.Length() && i<32; ++i)
            pw[i] = (ub4)(wxChar)pwd[i];

        for (int j=0; j<32; ++j)
            result += wxString::Format("%08lx", pw[j] ^ ctxM.randrsl[j]);
        return result;
    }

    wxString deCipher(const wxString& cipher)
    {
        wxString result;
        for (int i=0; i<32; ++i)
        {
            ub4 value;
            cipher.Mid(i*8, 8).ToULong(&value, 16);
            result += wxChar(value ^ ctxM.randrsl[i]);
        }
        return result;
    }
};

#endif
