#include "JPEGVideoSource.hh"

#define YUV422 0
#define YUV420 1
#define Q_FACTOR 128

class CVI_JPEGVideoSource : public JPEGVideoSource {
    public:
        static CVI_JPEGVideoSource* createNew(UsageEnvironment& env, FramedSource* source);
        virtual void doGetNextFrame();
        static void afterGettingFrameSub(void* clientData, unsigned frameSize,unsigned numTruncatedBytes,struct timeval presentationTime,unsigned durationInMicroseconds);
        void afterGettingFrame(unsigned frameSize,unsigned numTruncatedBytes,struct timeval presentationTime,unsigned durationInMicroseconds);

        virtual u_int8_t type();
        virtual u_int8_t qFactor();
        virtual u_int8_t width();
        virtual u_int8_t height();
        virtual u_int8_t const *quantizationTables(u_int8_t &precision, u_int16_t &length);
    protected:
        CVI_JPEGVideoSource(UsageEnvironment& env, FramedSource* source);
        virtual ~CVI_JPEGVideoSource();

    protected:
        FramedSource *_inputSource;
        u_int8_t _width;
        u_int8_t _height;
        u_int8_t _qTable[Q_FACTOR];
        bool _qTable0Init;
        bool _qTable1Init;
};

CVI_JPEGVideoSource* CVI_JPEGVideoSource::createNew(UsageEnvironment& env, FramedSource* source)
{
    return new CVI_JPEGVideoSource(env, source);
}

CVI_JPEGVideoSource::CVI_JPEGVideoSource(UsageEnvironment& env, FramedSource* source):
    JPEGVideoSource(env), _inputSource(source),
    _width(0), _height(0), _qTable0Init(false), _qTable1Init(false)
{
}

CVI_JPEGVideoSource::~CVI_JPEGVideoSource()
{
    if (_inputSource) {
        Medium::close(_inputSource);
    }
}

u_int8_t CVI_JPEGVideoSource::type()
{
    return YUV420;
}

u_int8_t CVI_JPEGVideoSource::qFactor()
{
    return Q_FACTOR;
}

u_int8_t CVI_JPEGVideoSource::width()
{
    return _width;
}

u_int8_t CVI_JPEGVideoSource::height()
{
    return _height;
}

void CVI_JPEGVideoSource::afterGettingFrameSub(void *clientData, unsigned frameSize, unsigned numTruncatedBytes, struct timeval presentationTime, unsigned durationInMicroseconds)
{
    CVI_JPEGVideoSource* source = (CVI_JPEGVideoSource*)clientData;
    source->afterGettingFrame(frameSize, numTruncatedBytes, presentationTime, durationInMicroseconds);
}

void CVI_JPEGVideoSource::doGetNextFrame()
{
    if (_inputSource) {
        _inputSource->getNextFrame(fTo, fMaxSize, afterGettingFrameSub, this, FramedSource::handleClosure, this);
    }
}

void CVI_JPEGVideoSource::afterGettingFrame(unsigned frameSize, unsigned numTruncatedBytes, struct timeval presentationTime, unsigned durationInMicroseconds) 
{
    int headerSize = 0;
    bool headerOk = false;

    // RFC 2435
    // https://tools.ietf.org/html/rfc2435
    for (unsigned int i = 0; i < frameSize ; i++) {
        // SOF
        if ((i+8) < frameSize  && fTo[i] == 0xFF && fTo[i+1] == 0xC0) {
            _height = (fTo[i+5]<<5) | (fTo[i+6]>>3);
            _width = (fTo[i+7]<<5) | (fTo[i+8]>>3);
        }
        // DQT
        if ((i+5+64) < frameSize && fTo[i] == 0xFF && fTo[i+1] == 0xDB) {
            if (fTo[i+4] ==0) {
                memcpy(_qTable, fTo + i + 5, 64);
                _qTable0Init = true;
            } else if (fTo[i+4] == 1) {
                memcpy(_qTable + 64, fTo + i + 5, 64);
                _qTable1Init = true;
            }
        }
        // End of header
        if ((i+1) < frameSize && fTo[i] == 0x3F && fTo[i+1] == 0x00) {
            headerOk = true;
            headerSize = i+2;
            break;
        }
    }

    if (headerOk) {
        fFrameSize = frameSize - headerSize;
        memcpy(fTo, fTo + headerSize, fFrameSize);
    }

    fNumTruncatedBytes = numTruncatedBytes;
    fPresentationTime = presentationTime;
    fDurationInMicroseconds = durationInMicroseconds;
    afterGetting(this);
}

u_int8_t const *CVI_JPEGVideoSource::quantizationTables(u_int8_t &precision, u_int16_t &length)
{
	length = 0;
	precision = 0;

	if (_qTable0Init && _qTable1Init) {
		precision = 8;
		length = sizeof(_qTable);
	}

	return _qTable;
}
