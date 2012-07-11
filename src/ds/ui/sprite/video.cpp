#include "Video.h"

namespace ds {
namespace ui {

Video::Video( SpriteEngine& engine, const std::string &filename )
    : inherited(engine)
    , mLooping(false)
{
    setTransparent(false);
    try
    {
        mMovie = ci::qtime::MovieGl( filename );
        mMovie.setLoop(mLooping);
        mMovie.play();
    }
    catch (...)
    {
        return;
    }

    Sprite::setSize(static_cast<float>(mMovie.getWidth()), static_cast<float>(mMovie.getHeight()));
}

Video::~Video()
{

}

void Video::drawLocal()
{
    if ( mMovie )
        mFrameTexture = mMovie.getTexture();
    if ( mFrameTexture )
        ci::gl::draw(mFrameTexture);
}

void Video::setSize( float width, float height )
{
    setScale( width / getWidth(), height / getHeight() );
}

void Video::loadVideo( const std::string &filename )
{
    try
    {
        mMovie = ci::qtime::MovieGl( filename );
        mMovie.setLoop(mLooping);
        mMovie.play();
    }
    catch (...)
    {
        return;
    }

    float prevWidth = getWidth() * getScale().x;
    float prevHeight = getHeight() * getScale().y;

    Sprite::setSize(static_cast<float>(mMovie.getWidth()), static_cast<float>(mMovie.getHeight()));
    setSize(prevWidth, prevHeight);
}

void Video::play()
{
    if ( mMovie )
        mMovie.play();
}

void Video::stop()
{
    if ( mMovie )
    {
        mMovie.stop();
        mMovie.reset();
    }
}

void Video::pause()
{
    if ( mMovie )
        mMovie.stop();
}

void Video::seek( float t )
{
    if ( mMovie )
        mMovie.seekToTime(t);
}

float Video::duration() const
{
    if ( mMovie )
        return mMovie.getDuration();
    return 0.0f;
}

bool Video::isPlaying() const
{
    if ( mMovie )
        return mMovie.isPlaying();
    return false;
}

void Video::loop( bool flag )
{
    mLooping = flag;
    if ( mMovie )
        mMovie.setLoop(mLooping);
}

bool Video::isLooping() const
{
    return mLooping;
}

} // namespace ui
} // namespace ds