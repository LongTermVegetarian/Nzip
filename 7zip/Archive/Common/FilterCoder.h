// FilterCoder.h

#ifndef __FILTERCODER_H
#define __FILTERCODER_H

#include "../../../Common/MyCom.h"
#include "../../ICoder.h"
#include "../../IPassword.h"

#define MY_QUERYINTERFACE_ENTRY_AG(i, sub0, sub) if (iid == IID_ ## i) \
{ if (!sub) RINOK(sub0->QueryInterface(IID_ ## i, (void **)&sub)) \
*outObject = (void *)(i *)this; AddRef(); return S_OK; } 

class CFilterCoder:
  public ICompressCoder,
  public ICompressSetInStream,
  public ISequentialInStream,
  public ICompressSetOutStream,
  public ISequentialOutStream,
  public IOutStreamFlush,

  #ifndef _NO_CRYPTO
  public ICryptoSetPassword,
  #endif
  #ifndef EXTRACT_ONLY
  public ICompressWriteCoderProperties,
  #endif
  public ICompressSetDecoderProperties,
  public CMyUnknownImp
{
protected:
  Byte *_buffer;
  CMyComPtr<ISequentialInStream> _inStream;
  CMyComPtr<ISequentialOutStream> _outStream;
  bool _outSizeIsDefined;
  UInt64 _outSize;
  UInt64 _nowPos64;

  UInt32 _bufferPos;
  UInt32 _convertedPosBegin;
  UInt32 _convertedPosEnd;
  void Init() 
  { 
    Filter->Init(); 
    _nowPos64 = 0;
    _outSizeIsDefined = false;
  }

  CMyComPtr<ICryptoSetPassword> _setPassword;
  CMyComPtr<ICompressWriteCoderProperties> _writeCoderProperties;
  CMyComPtr<ICompressSetDecoderProperties> _setDecoderProperties;
public:
  CMyComPtr<ICompressFilter> Filter;

  CFilterCoder();
  ~CFilterCoder();
  HRESULT WriteWithLimit(ISequentialOutStream *outStream, UInt32 size);
  bool NeedMore() const  
    { return (!_outSizeIsDefined || (_nowPos64 < _outSize)); }

public:
  MY_QUERYINTERFACE_BEGIN
    MY_QUERYINTERFACE_ENTRY(ICompressCoder)
    MY_QUERYINTERFACE_ENTRY(ICompressSetInStream)
    MY_QUERYINTERFACE_ENTRY(ISequentialInStream)

    MY_QUERYINTERFACE_ENTRY(ICompressSetOutStream)
    MY_QUERYINTERFACE_ENTRY(ISequentialOutStream)
    MY_QUERYINTERFACE_ENTRY(IOutStreamFlush)

    #ifndef _NO_CRYPTO
    MY_QUERYINTERFACE_ENTRY_AG(ICryptoSetPassword, Filter, _setPassword)
    #endif

    #ifndef EXTRACT_ONLY
    MY_QUERYINTERFACE_ENTRY_AG(ICompressWriteCoderProperties, Filter, _writeCoderProperties)
    #endif

    MY_QUERYINTERFACE_ENTRY_AG(ICompressSetDecoderProperties, Filter, _setDecoderProperties)
  MY_QUERYINTERFACE_END
  MY_ADDREF_RELEASE
  STDMETHOD(Code)(ISequentialInStream *inStream,
      ISequentialOutStream *outStream, const UInt64 *inSize, const UInt64 *outSize,
      ICompressProgressInfo *progress);
  STDMETHOD(ReleaseInStream)();
  STDMETHOD(SetInStream)(ISequentialInStream *inStream);
  STDMETHOD(Read)(void *data, UInt32 size, UInt32 *processedSize); \
  STDMETHOD(ReadPart)(void *data, UInt32 size, UInt32 *processedSize); \
  STDMETHOD(SetOutStream)(ISequentialOutStream *outStream);
  STDMETHOD(ReleaseOutStream)();
  STDMETHOD(Write)(const void *data, UInt32 size, UInt32 *processedSize);
  STDMETHOD(WritePart)(const void *data, UInt32 size, UInt32 *processedSize);
  STDMETHOD(Flush)();

  #ifndef _NO_CRYPTO
  STDMETHOD(CryptoSetPassword)(const Byte *data, UInt32 size);
  #endif
  #ifndef EXTRACT_ONLY
  STDMETHOD(WriteCoderProperties)(ISequentialOutStream *outStream);
  #endif
  STDMETHOD(SetDecoderProperties)(ISequentialInStream *inStream);
};

class CInStreamReleaser
{
public:
  CFilterCoder *FilterCoder;
  CInStreamReleaser(): FilterCoder(0) {}
  ~CInStreamReleaser() { if (FilterCoder) FilterCoder->ReleaseInStream(); }
};

class COutStreamReleaser
{
public:
  CFilterCoder *FilterCoder;
  COutStreamReleaser(): FilterCoder(0) {}
  ~COutStreamReleaser() { if (FilterCoder) FilterCoder->ReleaseOutStream(); }
};

#endif