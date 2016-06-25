#include "basicRSA.h"
inline DWORD BigNumberAdd(DWORD C[], const DWORD A[], const DWORD B[], const UINT nSize)
{	
	DWORD k=0; // carry 
	for (UINT i = 0; i < nSize; i++)
	{
		C[i] = A[i] + k;
		if(C[i]>=k)// detect overflow and set k
			k=0;
		else
			k=1;

		C[i] += B[i];
		if (C[i] < B[i]) // Detect overflow again. 
			k++;	
	}
	return k;
}

int BigNumberCompare(const DWORD a[], const DWORD b[], UINT nSize)
{
	if ( nSize <= 0 ) 
		return 0;
	while (nSize--)
	{
		if (a[nSize] > b[nSize])
			return 1;	// Grater than 
		if (a[nSize] < b[nSize])
			return -1;	// Less Than 
	}
	return 0;// equal 
}
inline UINT BigNumberSizeof(const DWORD A[], UINT nSize)
{
	while ( nSize-- )
	{
		if ( A[nSize] != 0 )
			return (++nSize);
	}
	return 0;
}

inline DWORD BigNumberShiftLeft(DWORD a[], const DWORD *b, UINT x, UINT nSize)
{
	DWORD mask, carry, nextcarry;
	UINT i=0;

	if ( x >= sizeof(DWORD)*8 )
		return 0;

	mask = _HIBITMASK_;
	for (i = 1; i < x; i++)
	{
		mask = (mask >> 1) | mask;
	}
	if (x == 0) mask = 0x0;

	UINT y = (sizeof(DWORD)*8) - x;
	carry = 0;
	for (i = 0; i < nSize; i++)
	{
		nextcarry = (b[i] & mask) >> y;
		a[i] = b[i] << x | carry;
		carry = nextcarry;
	}

	return carry;
}

inline DWORD BigNumberShiftRight(DWORD a[], const DWORD *b, DWORD x, DWORD nSize)
{

	DWORD mask, carry, nextcarry;
	UINT i=0;
	// be safe. 
	if ( x >= (sizeof(DWORD)*8) )
		return 0;

	// Create mask
	mask = 0x1;
	for (i = 1; i < x; i++)
	{
		mask = (mask << 1) | mask;
	}
	if (x == 0) mask = 0x0;

	UINT y = (sizeof(DWORD)*8) - x;
	carry = 0;
	i = nSize;
	while (i--)
	{
		nextcarry = (b[i] & mask) << y;
		a[i] = b[i] >> x | carry;
		carry = nextcarry;
	}

	return carry;	
}

inline int BigNumberMultiplyHelper(DWORD p[2], const DWORD x, const DWORD y)
{
	__int64 o = x;
	o *= y;
	p[0] = o &0xffffffff;
	p[1] = (o &0xffffffff00000000)>>32;
	return p[0];
}


inline DWORD BigNumberMultSub(DWORD wn, DWORD w[], const DWORD v[], DWORD q, UINT n)
{
	DWORD k, t[2];
	UINT i;

	if ( q == 0 )	// Nothing to do 
		return wn;

	k = 0;

	for (i = 0; i < n; i++)
	{
		BigNumberMultiplyHelper(t, q, v[i]);
		w[i] -= k;
		if (w[i] > _MAXIMUMNR_ - k)
			k = 1;
		else
			k = 0;
		w[i] -= t[0];
		if (w[i] > _MAXIMUMNR_ - t[0])
			k++;
		k += t[1];
	}

	// Cope with Wn not stored in array w[0..n-1] 
	wn -= k;

	return wn;	
}
DWORD BigNumberMultiply(DWORD C[], const DWORD A[], const DWORD B[], const UINT nSize)
{
	DWORD  k, tmp[2];
	UINT m, n;
	m = n = nSize;
	for ( UINT i = 0; i < 2 * m; i++)
		C[i] = 0;

	for ( UINT j = 0; j < n; j++)
	{
		if (B[j] == 0) // Zero multiplication ?
		{
			C[j + m] = 0;
		}
		else
		{
			k = 0;
			for (UINT i = 0; i < m; i++)
			{
				// Perform t = A[i] * B[i]+C[i+j] + k
				// use Help function for code cleaness. 
				BigNumberMultiplyHelper(tmp, A[i], B[j]);
				tmp[0] += k;
				if (tmp[0] < k) // Overflow? 
					tmp[1]++;
				tmp[0] += C[i+j];
				if (tmp[0] < C[i+j])
					tmp[1]++;
				k = tmp[1];
				C[i+j] = tmp[0];

			}	
			C[j+m] = k;
		}
	}	
	return 0;
}

inline void BigNumberMultSubHelper(DWORD uu[], DWORD qhat, DWORD v1, DWORD v0)
{
	DWORD p0, p1, t;
	p0 = qhat * v0;
	p1 = qhat * v1;
	t = p0 + TOHIGH(LOHALF(p1));
	uu[0] -= t;
	if (uu[0] > _MAXIMUMNR_ - t)
		uu[1]--;	// Borrow
	uu[1] -= HIHALF(p1);

}
inline DWORD BigNumberDivideHelper(DWORD *q, DWORD *r, const DWORD u[], DWORD v)
{
	DWORD q2;
	DWORD qhat, rhat, t, v0, v1, u0, u1, u2, u3;
	DWORD uu[2];
	DWORD B= _MAXHALFNR_+1;

	if (!(v & _HIBITMASK_))
	{	
		*q = *r = 0;
		return _MAXIMUMNR_;
	}

	v0 = LOHALF(v);
	v1 = HIHALF(v);
	u0 = LOHALF(u[0]);
	u1 = HIHALF(u[0]);
	u2 = LOHALF(u[1]);
	u3 = HIHALF(u[1]);
	qhat = (u3 < v1 ? 0 : 1);
	if (qhat > 0)
	{
		rhat = u3 - v1;
		t = TOHIGH(rhat) | u2;
		if (v0 > t)
			qhat--;
	}
	uu[0] = u[1];
	uu[1] = 0;		
	if (qhat > 0)
	{

		BigNumberMultSubHelper(uu, qhat, v1, v0);
		if (HIHALF(uu[1]) != 0)
		{
			uu[0] += v;
			uu[1] = 0;
			qhat--;
		}
	}
	q2 = qhat;
	t = uu[0];
	qhat = t / v1;
	rhat = t - qhat * v1;
	t = TOHIGH(rhat) | u1;
	if ( (qhat == B) || (qhat * v0 > t) )
	{
		qhat--;
		rhat += v1;
		t = TOHIGH(rhat) | u1;
		if ((rhat < B) && (qhat * v0 > t))
			qhat--;
	}
	uu[1] = HIHALF(uu[0]);	
	uu[0] = TOHIGH(LOHALF(uu[0])) | u1;	
	BigNumberMultSubHelper(uu, qhat, v1, v0);
	if ( HIHALF(uu[1]) != 0 )
	{	
		qhat--;
		uu[0] += v;
		uu[1] = 0;
	}
	*q = TOHIGH(qhat);
	t = uu[0];
	qhat = t / v1;
	rhat = t - qhat * v1;
	t = TOHIGH(rhat) | u0;
	if ( (qhat == B) || (qhat * v0 > t) )
	{
		qhat--;
		rhat += v1;
		t = TOHIGH(rhat) | u0;
		if ((rhat < B) && (qhat * v0 > t))
			qhat--;
	}
	uu[1] = HIHALF(uu[0]);
	uu[0] = TOHIGH(LOHALF(uu[0])) | u0;	
	BigNumberMultSubHelper(uu, qhat, v1, v0);
	if (HIHALF(uu[1]) != 0)
	{	
		qhat--;
		uu[0] += v;
		uu[1] = 0;
	}
	*q |= LOHALF(qhat);
	*r = uu[0];
	return q2;
}

inline DWORD BigNumberDividedw(DWORD q[], const DWORD u[], DWORD v, UINT nSize)
{
	UINT j;
	DWORD t[2], r;
	UINT shift;
	DWORD bitmask, overflow, *uu;

	if (nSize == 0) return 0;
	if (v == 0)	return 0;

	bitmask = _HIBITMASK_;
	for (shift = 0; shift < (sizeof(DWORD)*8); shift++)
	{
		if (v & bitmask)
			break;
		bitmask >>= 1;
	}	
	v <<= shift;
	overflow = BigNumberShiftLeft(q, u, shift, nSize);
	uu = q;
	r = overflow;	
	j = nSize;
	while (j--)
	{
		t[0] = uu[j];
		t[1] = r;
		overflow = BigNumberDivideHelper(&q[j], &r, t, v);
	}
	r >>= shift;	
	return r;
}

inline int BigNumberSquare(DWORD w[], const DWORD x[], UINT nSize)
{
	DWORD k, p[2], u[2], cbit, carry;
	UINT i, j, t, i2, cpos;
	t = nSize;

	i2 = t << 1;

	for (i = 0; i < i2; i++)
		w[i] = 0;

	carry = 0;
	cpos = i2-1;

	for (i = 0; i < t; i++)
	{

		i2 = i << 1; 
		BigNumberMultiplyHelper(p, x[i], x[i]);
		p[0] += w[i2];

		if (p[0] < w[i2])
			p[1]++;
		k = 0;	
		if ( i2 == cpos && carry )
		{
			p[1] += carry;
			if (p[1] < carry)
				k++;
			carry = 0;
		}

		u[0] = p[1];
		u[1] = k;
		w[i2] = p[0];

		k = 0;
		for ( j = i+1; j < t; j++ )
		{
			BigNumberMultiplyHelper(p, x[j], x[i]);
			cbit = (p[0] & _HIBITMASK_) != 0;
			k =  (p[1] & _HIBITMASK_) != 0;
			p[0] <<= 1;
			p[1] <<= 1;
			p[1] |= cbit;

			p[0] += u[0];
			if (p[0] < u[0])
			{
				p[1]++;
				if (p[1] == 0)
					k++;
			}
			p[1] += u[1];
			if (p[1] < u[1])
				k++;

			p[0] += w[i+j];
			if (p[0] < w[i+j])
			{
				p[1]++;
				if (p[1] == 0)
					k++;
			}
			if ((i+j) == cpos && carry)
			{
				p[1] += carry;
				if (p[1] < carry)
					k++;
				carry = 0;
			}
			u[0] = p[1];
			u[1] = k;
			w[i+j] = p[0];
		}

		carry = u[1];
		w[i+t] = u[0];
		cpos = i+t;
	}	
	return 0;
}

inline int BigNumberQhatTooBigHelper(DWORD qhat, DWORD rhat, DWORD vn2, DWORD ujn2)
{
	DWORD t[2];
	BigNumberMultiplyHelper(t, qhat, vn2);
	if ( t[1] < rhat )
		return 0;
	else if ( t[1] > rhat )
		return 1;
	else if ( t[0] > ujn2 )
		return 1;
	return 0;	
}
int BigNumberDivide(DWORD q[], DWORD r[], const DWORD u[], UINT usize, DWORD v[], UINT vsize)
{
	UINT shift;
	int n, m, j;
	DWORD bitmask, overflow;
	DWORD qhat, rhat, t[2];
	DWORD *uu, *ww;
	int qhatOK, cmp;
	BigNumberSetZero(q, usize);
	BigNumberSetZero(r, usize);

	//  Work out exact sizes of u and v 
	n = (int)BigNumberSizeof(v, vsize);
	m = (int)BigNumberSizeof(u, usize);
	m -= n;

	// special cases 
	if ( n == 0 )
		return -1;	// divide by zero

	if ( n == 1 )
	{	// Short div is better 
		r[0] = BigNumberDividedw(q, u, v[0], usize);
		return 0;
	}

	if ( m < 0 )
	{   // set r=u
		BigNumberSetEqual(r, u, usize);
		return 0;
	}

	if ( m == 0 )
	{
		cmp = BigNumberCompare(u, v, (UINT)n);
		if (cmp < 0)
		{
			BigNumberSetEqual(r, u, usize);
			return 0;
		}
		else if (cmp == 0)
		{
			BigNumberSetEqualdw(q, 1, usize);
			return 0;
		}
	}

	bitmask =  _HIBITMASK_;
	for ( shift = 0; shift < 32; shift++ )
	{
		if (v[n-1] & bitmask)
			break;
		bitmask >>= 1;
	}

	overflow = BigNumberShiftLeft(v, v, shift, n);
	overflow = BigNumberShiftLeft(r, u, shift, n + m);
	t[0] = overflow;	
	uu = r;	
	for ( j = m; j >= 0; j-- )
	{
		qhatOK = 0;
		t[1] = t[0];
		t[0] = uu[j+n-1];
		overflow = BigNumberDivideHelper(&qhat, &rhat, t, v[n-1]);
		if ( overflow )
		{	
			rhat = uu[j+n-1];
			rhat += v[n-1];
			qhat = _MAXIMUMNR_;
			if (rhat < v[n-1])	
				qhatOK = 1;
		}
		if (qhat && !qhatOK && BigNumberQhatTooBigHelper(qhat, rhat, v[n-2], uu[j+n-2]))
		{	
			rhat += v[n-1];
			qhat--;
			if (!(rhat < v[n-1]))
				if (BigNumberQhatTooBigHelper(qhat, rhat, v[n-2], uu[j+n-2]))
					qhat--;
		}
		ww = &uu[j];
		overflow = BigNumberMultSub(t[1], ww, v, qhat, (UINT)n);
		q[j] = qhat;
		if (overflow)
		{	
			q[j]--;
			overflow = BigNumberAdd(ww, ww, v, (UINT)n);
		}
		t[0] = uu[j+n-1];	
	}	
	for (j = n; j < m+n; j++)
		uu[j] = 0;
	BigNumberShiftRight(r, r, shift, n);
	BigNumberShiftRight(v, v, shift, n);
	return 0;
}
inline int BigNumberModuloTmp(DWORD r[], const DWORD u[], UINT nUSize, DWORD v[], UINT nVSize, DWORD tqq[], DWORD trr[])
{
	BigNumberDivide(tqq, trr, u, nUSize, v, nVSize);
	BigNumberSetEqual(r, trr, nVSize);	
	return 0;
}
inline int BigNumberModSquareTmp(DWORD a[], const DWORD x[], DWORD m[], UINT nSize, DWORD temp[], DWORD tqq[], DWORD trr[])
{
	//x*x
	BigNumberSquare(temp, x, nSize);

	// Then modulo m
	BigNumberModuloTmp(a, temp, nSize * 2, m, nSize, tqq, trr);
	return 0;
}
inline int BigNumberMultTmp(DWORD a[], const DWORD x[], const DWORD y[], DWORD m[], UINT nSize, DWORD temp[], DWORD tqq[], DWORD trr[])
{
	BigNumberMultiply(temp, x, y, nSize);
	BigNumberModuloTmp(a, temp, nSize * 2, m, nSize, tqq, trr);
	return 0; 
}


int RSA(DWORD out[], const DWORD in[], const DWORD private_key_or_e[], const DWORD public_key[], UINT nSize)
{
	// Be safe
	if ( nSize <= 0 ) 
		return -1;

	DWORD mask;
	UINT n;
	DWORD *t1, *t2, *t3, *tm, *y; //temporary variables 

	// Create some temporary variables
	const UINT nn = nSize * 2;

	t1 = BigNumberAlloc(nn);
	if(t1==NULL)
	{
		return -1;
	}
	t2 = BigNumberAlloc(nn);
	if(t2==NULL)
	{
		BigNumberFree(&t1);
		return -1;
	}
	t3 = BigNumberAlloc(nn);
	if(t3==NULL)
	{
		BigNumberFree(&t1);
		BigNumberFree(&t2);
		return -1;
	}
	tm = BigNumberAlloc(nSize);
	if(tm==NULL)
	{
		BigNumberFree(&t1);
		BigNumberFree(&t2);
		BigNumberFree(&t3);
		return -1;
	}
	y = BigNumberAlloc(nSize);
	if(y==NULL)
	{
		BigNumberFree(&t1);
		BigNumberFree(&t2);
		BigNumberFree(&t3);
		BigNumberFree(&tm);
		return -1;
	}

	BigNumberSetEqual(tm, public_key, nSize);

	//  Find second-most significant bit in private_key_or_e 
	n = BigNumberSizeof(private_key_or_e, nSize);
	for (mask = _HIBITMASK_; mask > 0; mask >>= 1)
	{
		if (private_key_or_e[n-1] & mask)
			break;
	}

	// next bitmask 
	if ( mask==1 )
	{
		mask=_HIBITMASK_;
		n--;
	}else
		mask >>=1; 

	BigNumberSetEqual(y, in, nSize);

	while ( n )
	{

		BigNumberModSquareTmp(y, y, tm, nSize, t1, t2, t3);	
		if (mask & private_key_or_e[n-1])
			BigNumberMultTmp(y, y, in, tm, nSize, t1, t2, t3);

		// Move to next bit
		// next bitmask 
		if ( mask==1 )
		{
			mask=_HIBITMASK_;
			n--;
		}else
			mask >>=1; 
	}

	BigNumberSetEqual(out, y, nSize);

	BigNumberFree(&t1);
	BigNumberFree(&t2);
	BigNumberFree(&t3);
	BigNumberFree(&tm);
	BigNumberFree(&y);
	return 0;
}