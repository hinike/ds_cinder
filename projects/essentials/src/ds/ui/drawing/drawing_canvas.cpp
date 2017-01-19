#include "drawing_canvas.h"

#include <Poco/LocalDateTime.h>

#include <ds/app/app.h>
#include <ds/app/engine/engine.h>
#include <ds/app/blob_registry.h>
#include <ds/app/blob_reader.h>
#include "ds/data/data_buffer.h"
#include <ds/ui/sprite/dirty_state.h>
#include <ds/ui/sprite/sprite_engine.h>

#include <ds/debug/logger.h>
#include <ds/util/file_meta_data.h>
#include <ds/app/environment.h>

//#include <ds/gl/save_camera.h>
#include <cinder/ImageIo.h>

#include <cinder/Rand.h>

namespace {

const static std::string whiteboard_point_vert =
"#version 150\n"
"uniform mat4		ciModelViewProjection;\n"
"in vec4			ciPosition;\n"
"in vec4			ciColor;\n"
"out vec4			oColor;\n"
"in vec2			ciTexCoord0;\n"
"out vec2			TexCoord0;\n"
"uniform vec4		vertexColor;\n"
"out vec4			brushColor;\n"

"void main(){\n"
"	gl_Position = ciModelViewProjection * ciPosition;\n"
"	TexCoord0 = ciTexCoord0;\n"
"	oColor = ciColor;\n"

	"brushColor = vertexColor;\n"
"}\n";

const static std::string whiteboard_point_frag =
"uniform sampler2D	tex0;\n"
"uniform float		opaccy;\n"
"in vec4			ciColor;\n"
"out vec4			oColor;\n"
"in vec2			TexCoord0;\n"
"in vec4			brushColor;\n"
"void main(){\n"
"oColor = texture2D(tex0, TexCoord0);\n"
"vec4 newColor = brushColor;\n"
"newColor.r *= brushColor.a * oColor.r;\n"
"newColor.g *= brushColor.a * oColor.g;\n"
"newColor.b *= brushColor.a * oColor.b;\n"
"newColor *= oColor.a;\n"
"newColor.r = 1.0;\n newColor.g=0.5;\n newColor.b=0.5;\n newColor.a=0.4;\n"
"oColor = newColor;\n"
//NEON EFFECTS!//"gl_FragColor.rgb = pow(gl_FragColor.rgb, vec3(1.0/2.2));"
"}\n";

static std::string whiteboard_point_name = "whiteboard_point";


const std::string opacityFrag =
"uniform sampler2D	tex0;\n"
"uniform float		opaccy;\n"
"in vec4			ciColor;\n"
"out vec4			oColor;\n"
"in vec2			TexCoord0;\n"
"void main()\n"
"{\n"
//"    oColor = vec4(1.0, 1.0, 1.0, 1.0);\n"
"    oColor = texture2D( tex0, TexCoord0 );\n"
"    oColor *= ciColor;\n"
"    oColor *= opaccy;\n"
"}\n";

const std::string vertShader =
"uniform mat4	ciModelViewProjection;\n"
"in vec4			ciPosition;\n"
"in vec4			ciColor;\n"
"out vec4			oColor;\n"
"in vec2			ciTexCoord0;\n"
"out vec2			TexCoord0;\n"
"void main()\n"
"{\n"
"	gl_Position = ciModelViewProjection * ciPosition;\n"
"	TexCoord0 = ciTexCoord0;\n"
"	oColor = ciColor;\n"
"}\n";

std::string shaderNameOpaccy = "opaccy_shader";
}

namespace ds {
namespace ui {

// Client/Server Stuff ------------------------------
namespace { // anonymous namespace
class Init {
public:
	Init() {
		ds::App::AddStartup( []( ds::Engine& e ) {
			e.installSprite(	[]( ds::BlobRegistry& r ){ds::ui::DrawingCanvas::installAsServer( r ); },
								[]( ds::BlobRegistry& r ){ds::ui::DrawingCanvas::installAsClient( r ); } );
		} );
	}
};
Init				INIT;
char				BLOB_TYPE				= 0;
const char			DRAW_POINTS_QUEUE_ATT	= 81;
const char			BRUSH_IMAGE_SRC_ATT		= 82;
const char			BRUSH_COLOR_ATT 		= 83;
const char			BRUSH_SIZE_ATT			= 84;
const DirtyState&	sPointsQueueDirty	 	= newUniqueDirtyState();
const DirtyState&	sBrushImagePathDirty	= newUniqueDirtyState();
const DirtyState&	sBrushColorDirty		= newUniqueDirtyState();
const DirtyState&	sBrushSizeDirty			= newUniqueDirtyState();

const int			MAX_SERIALIZED_POINTS	= 100;
} // anonymous namespace

void DrawingCanvas::installAsServer(ds::BlobRegistry& registry) {
	BLOB_TYPE = registry.add([](BlobReader& r) {Sprite::handleBlobFromClient(r); });
}

void DrawingCanvas::installAsClient(ds::BlobRegistry& registry) {
	BLOB_TYPE = registry.add([](BlobReader& r) {Sprite::handleBlobFromServer<DrawingCanvas>(r); });
}
// -- Client/Server Stuff ----------------------------


DrawingCanvas::DrawingCanvas(ds::ui::SpriteEngine& eng, const std::string& brushImagePath)
	: ds::ui::Sprite(eng)
	, ds::ui::ImageOwner(eng)
	, mBrushSize(24.0f)
	, mBrushColor(1.0f, 0.0f, 0.0f, 0.5f)
	, mPointShader(whiteboard_point_vert, whiteboard_point_frag, whiteboard_point_name)
	, mBrushImage(nullptr)
	, mEraseMode(false)
	, mOutputShader(vertShader, opacityFrag, shaderNameOpaccy)
	, mDrawTexture()
{
	mBlobType = BLOB_TYPE;
	mOutputShader.loadShaders();

	mPointShader.loadShaders();
	mFboGeneral = std::move(mEngine.getFbo());
	DS_REPORT_GL_ERRORS();

	setBrushImage(brushImagePath);
	markAsDirty(sBrushSizeDirty);

	setBrushColor(ci::ColorA(1.0f, 0.3f, 0.3f, 0.7f));
	setSize(mEngine.getWorldWidth(), mEngine.getWorldHeight());
	setTransparent(false);
	setColor(ci::Color(1.0f, 1.0f, 1.0f));
	setUseShaderTexture(true);

	enable(true);
	enableMultiTouch(ds::ui::MULTITOUCH_INFO_ONLY);
	setProcessTouchCallback([this](ds::ui::Sprite*, const ds::ui::TouchInfo& ti){
		auto localPoint = globalToLocal(ti.mCurrentGlobalPoint);
		auto prevPoint = globalToLocal(ti.mCurrentGlobalPoint - ti.mDeltaPoint);
		if(ti.mPhase == ds::ui::TouchInfo::Added){
			//TODO: make sure it is necessary to make_pair
			mSerializedPointsQueue.push_back( std::make_pair(ci::vec2(localPoint), ci::vec2(localPoint)));
			renderLine(localPoint, localPoint);
			markAsDirty(sPointsQueueDirty);
		}
		if(ti.mPhase == ds::ui::TouchInfo::Moved){
			mSerializedPointsQueue.push_back( std::make_pair(ci::vec2(prevPoint), ci::vec2(localPoint)) );
			renderLine(prevPoint, localPoint);
			markAsDirty(sPointsQueueDirty);
		}
		// Don't let the queue get too large if there are no clients connected
		while (mSerializedPointsQueue.size() > MAX_SERIALIZED_POINTS)
			mSerializedPointsQueue.pop_front();
	});
}

void DrawingCanvas::setBrushColor(const ci::ColorA& brushColor){
	mBrushColor = brushColor;
	markAsDirty(sBrushColorDirty);
}

void DrawingCanvas::setBrushColor(const ci::Color& brushColor){
	mBrushColor.r = brushColor.r; mBrushColor.g = brushColor.g; mBrushColor.b = brushColor.b;
	markAsDirty(sBrushColorDirty);
}

void DrawingCanvas::setBrushOpacity(const float brushOpacity){
	mBrushColor.a = brushOpacity;
	markAsDirty(sBrushColorDirty);
}

const ci::ColorA& DrawingCanvas::getBrushColor(){
	return mBrushColor;
}

void DrawingCanvas::setBrushSize(const float brushSize){
	mBrushSize = brushSize;
	markAsDirty(sBrushSizeDirty);
}

const float DrawingCanvas::getBrushSize(){
	return mBrushSize;
}

void DrawingCanvas::setBrushImage(const std::string& imagePath){
	if (imagePath.empty()){
		DS_LOG_WARNING( "No brush image path supplied to drawing canvas" );
		return;
	}
	setImageFile(imagePath);
	markAsDirty( sBrushImagePathDirty );
}

void DrawingCanvas::clearCanvas(){
	auto w = getWidth();
	auto h = getHeight();

	if(!mDrawTexture || mDrawTexture->getWidth() != w || mDrawTexture->getHeight() != h) {
		ci::gl::Texture::Format format;
		format.setTarget(GL_TEXTURE_2D);
		format.setMagFilter(GL_LINEAR);
		format.setMinFilter(GL_LINEAR);
		mDrawTexture = ci::gl::Texture2d::create((int)w, (int)h, format);
		mDrawTexture->setTopDown(true);
	}

	//ds::gl::SaveCamera		save_camera;

	mFboGeneral->attach(mDrawTexture);
	mFboGeneral->begin();

	ci::Area fboBounds(0, 0, mFboGeneral->getWidth(), mFboGeneral->getHeight());
	//ci::gl::setViewport(fboBounds);
	ci::gl::ScopedViewport svp = ci::gl::ScopedViewport(0, 0, mFboGeneral->getWidth(), mFboGeneral->getHeight());
	//ci::gl::viewport(0, 0, mFboGeneral->getWidth(), mFboGeneral->getHeight());
	ci::CameraOrtho camera;
	camera.setOrtho(static_cast<float>(fboBounds.getX1()), static_cast<float>(fboBounds.getX2()), static_cast<float>(fboBounds.getY2()), static_cast<float>(fboBounds.getY1()), -1.0f, 1.0f);
	ci::gl::setMatrices(camera);

	ci::gl::clear(ci::ColorA(0.0f, 0.0f, 0.0f, 0.0f));

	//mPointShader.getShader().unbind();
	mFboGeneral->end();
	mFboGeneral->detach();
}

void DrawingCanvas::setEraseMode(const bool eraseMode){
	mEraseMode = eraseMode;
}

void DrawingCanvas::drawLocalClient(){
	// If any serialized points have been received from the server, draw them
	while (!mSerializedPointsQueue.empty()) {
		auto points = mSerializedPointsQueue.front();
		renderLine( ci::vec3( points.first,0 ), ci::vec3( points.second,0 ) );
		mSerializedPointsQueue.pop_front();
	}

	if(!mDrawTexture) return;

	if(mDrawTexture) {
	//	if(getBlendMode() == ds::ui::BlendMode::NORMAL){
	//		glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
		//}

		// ignore the "color" setting
		ci::gl::color(ci::Color::white());
		// The true flag is for premultiplied alpha, which this texture is
		ci::gl::enableAlphaBlending(true);
		ci::gl::GlslProgRef shaderBase = mOutputShader.getShader();
		if(shaderBase) {
			ci::gl::ScopedGlslProg scopedShaderBase(shaderBase);
			//shaderBase->bind();
			shaderBase->uniform("tex0", 0);
			shaderBase->uniform("opaccy", mDrawOpacity);
			mUniform.applyTo(shaderBase);


			if(getPerspective()){
				ci::gl::draw(mDrawTexture, ci::Rectf(0.0f, 0.0f, static_cast<float>(mDrawTexture->getWidth()), static_cast<float>(mDrawTexture->getHeight())));
			} else {
				ci::gl::draw(mDrawTexture, ci::Rectf(0.0f, static_cast<float>(mDrawTexture->getHeight()), static_cast<float>(mDrawTexture->getWidth()), 0.0f));
			}

		}
		else{

			if (getPerspective()){
				ci::gl::draw(mDrawTexture, ci::Rectf(0.0f, 0.0f, static_cast<float>(mDrawTexture->getWidth()), static_cast<float>(mDrawTexture->getHeight())));
			}
			else {
				ci::gl::draw(mDrawTexture, ci::Rectf(0.0f, static_cast<float>(mDrawTexture->getHeight()), static_cast<float>(mDrawTexture->getWidth()), 0.0f));
			}
		}

	}
}

void DrawingCanvas::renderLine(const ci::vec3& start, const ci::vec3& end){
	ci::gl::Texture2dRef brushTexture = getImageTexture();

	if(!brushTexture){
		DS_LOG_WARNING("No brush image texture when trying to render a line in Drawing Canvas");
		return;
	}
	
	brushTexture->setTopDown(true);

	float brushPixelStep = 3.0f;
	int vertexCount = 0;

	std::vector<ci::vec2> drawPoints;

	// Create a point for every pixel between start and end for smoothness
	int count = std::max<int>((int)ceilf(sqrtf((end.x - start.x) * (end.x - start.x) + (end.y - start.y) * (end.y - start.y)) / (float)brushPixelStep), 1);
	for(int i = 0; i < count; ++i) {
		drawPoints.push_back(ci::vec2(start.x + (end.x - start.x) * ((float)i / (float)count), start.y + (end.y - start.y) * ((float)i / (float)count)));
	}

	int w = (int)floorf(getWidth());
	int h = (int)floorf(getHeight());

	if(!mDrawTexture || mDrawTexture->getWidth() != w || mDrawTexture->getHeight() != h) {
		ci::gl::Texture::Format format;
		format.setTarget(GL_TEXTURE_2D);
		format.setMagFilter(GL_LINEAR);
		format.setMinFilter(GL_LINEAR);
		mDrawTexture = ci::gl::Texture2d::create(w, h, format);
		mDrawTexture->setTopDown(true);
	}

	//ds::gl::SaveCamera		save_camera;

	//ci::gl::SaveFramebufferBinding bindingSaver;
	mFboGeneral->attach(mDrawTexture);
	mFboGeneral->begin();
	mDrawTexture->bind(0);

	ci::Area fboBounds(0, 0, mFboGeneral->getWidth(), mFboGeneral->getHeight());
	//ci::gl::setViewport(fboBounds);
	ci::gl::ScopedViewport svp = ci::gl::ScopedViewport(0, 0, mFboGeneral->getWidth(), mFboGeneral->getHeight());
//	ci::gl::viewport();
	ci::CameraOrtho camera;
	camera.setOrtho(static_cast<float>(fboBounds.getX1()), static_cast<float>(fboBounds.getX2()), static_cast<float>(fboBounds.getY2()), static_cast<float>(fboBounds.getY1()), -1.0f, 1.0f);
	ci::gl::setMatrices(camera);


	mPointShader.getShader()->bind();
	mPointShader.getShader()->uniform("tex0", 10);
	mPointShader.getShader()->uniform("vertexColor", mBrushColor);

	glEnable(GL_BLEND);
	glBlendEquation(GL_FUNC_ADD);

	if(mEraseMode){
		glBlendFunc(GL_ZERO, GL_ONE_MINUS_SRC_ALPHA);
	} else {
		glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
	}
	//TODO: fix it! whatever it is.
	{
		ci::gl::ScopedTextureBind scopedTextureBursh(brushTexture);
		brushTexture->bind(10);

	/*	ci::gl::SaveTextureBindState saveBindState(brushTexture->getTarget());
		ci::gl::BoolState saveEnabledState(brushTexture->getTarget());
		ci::gl::ClientBoolState vertexArrayState(GL_VERTEX_ARRAY);
		ci::gl::ClientBoolState texCoordArrayState(GL_TEXTURE_COORD_ARRAY_BUFFER_BINDING_ARB);

		glEnableClientState(GL_VERTEX_ARRAY);
		GLfloat verts[8];
		glVertexPointer(2, GL_FLOAT, 0, verts);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		GLfloat texCoords[8];
		glTexCoordPointer(2, GL_FLOAT, 0, texCoords);
		*/

		auto srcArea = brushTexture->getBounds();
		const ci::Rectf srcCoords = brushTexture->getAreaTexCoords(srcArea);

		float widdy = mBrushSize;
		float hiddy = mBrushSize / ((float)brushTexture->getWidth() / (float)brushTexture->getHeight());

		for(auto it : drawPoints){


			ci::Rectf destRect = ci::Rectf(it.x - widdy / 2.0f, it.y - hiddy / 2.0f, it.x + widdy / 2.0f, it.y + hiddy / 2.0f);
			ci::gl::draw(brushTexture, ci::Area(srcCoords), destRect);

			/*

			verts[0 * 2 + 0] = destRect.getX2(); verts[0 * 2 + 1] = destRect.getY1();
			verts[1 * 2 + 0] = destRect.getX1(); verts[1 * 2 + 1] = destRect.getY1();
			verts[2 * 2 + 0] = destRect.getX2(); verts[2 * 2 + 1] = destRect.getY2();
			verts[3 * 2 + 0] = destRect.getX1(); verts[3 * 2 + 1] = destRect.getY2();

			texCoords[0 * 2 + 0] = srcCoords.getX2(); texCoords[0 * 2 + 1] = srcCoords.getY1();
			texCoords[1 * 2 + 0] = srcCoords.getX1(); texCoords[1 * 2 + 1] = srcCoords.getY1();
			texCoords[2 * 2 + 0] = srcCoords.getX2(); texCoords[2 * 2 + 1] = srcCoords.getY2();
			texCoords[3 * 2 + 0] = srcCoords.getX1(); texCoords[3 * 2 + 1] = srcCoords.getY2();
			

			glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
			*/
		}

		//brushTexture->unbind(10);
	}

	mDrawTexture->unbind(0);

	//mPointShader.getShader()->u
	mFboGeneral->end();
	mFboGeneral->detach();
	DS_REPORT_GL_ERRORS();
}

void DrawingCanvas::writeAttributesTo(DataBuffer& buf) {
	Sprite::writeAttributesTo(buf);

	if (mDirty.has(sBrushImagePathDirty)){
		buf.add(BRUSH_IMAGE_SRC_ATT);
		mImageSource.writeTo(buf);
	}
	if (mDirty.has(sBrushColorDirty)){
		buf.add(BRUSH_COLOR_ATT);
		buf.add(mBrushColor.r);
		buf.add(mBrushColor.g);
		buf.add(mBrushColor.b);
		buf.add(mBrushColor.a);
	}
	if (mDirty.has(sBrushSizeDirty)){
		buf.add(BRUSH_SIZE_ATT);
		buf.add<float>(mBrushSize);
	}
	if (mDirty.has(sPointsQueueDirty)){
		buf.add(DRAW_POINTS_QUEUE_ATT);
		buf.add<uint32_t>(mSerializedPointsQueue.size());
		for( auto &pair : mSerializedPointsQueue ) {
			buf.add<float>(pair.first.x);
			buf.add<float>(pair.first.y);
			buf.add<float>(pair.second.x);
			buf.add<float>(pair.second.y);
		}
		mSerializedPointsQueue.clear();
	}

}

void DrawingCanvas::readAttributeFrom(const char attrid, DataBuffer& buf){
	if (attrid == BRUSH_IMAGE_SRC_ATT) {
		mImageSource.readFrom(buf);
	}
	else if (attrid == BRUSH_COLOR_ATT) {
		mBrushColor.r = buf.read<float>();
		mBrushColor.g = buf.read<float>();
		mBrushColor.b = buf.read<float>();
		mBrushColor.a = buf.read<float>();
	}
	else if (attrid == BRUSH_SIZE_ATT) {
		mBrushSize = buf.read<float>();
	}
	else if (attrid == DRAW_POINTS_QUEUE_ATT) {
		uint32_t count = buf.read<uint32_t>();
		ci::vec2 p1, p2;
		for (uint32_t i = 0; i<count; i++) {
			p1.x = buf.read<float>();
			p1.y = buf.read<float>();
			p2.x = buf.read<float>();
			p2.y = buf.read<float>();
			mSerializedPointsQueue.push_back( std::make_pair(p1, p2) );
		}
	}
	else {
		Sprite::readAttributeFrom(attrid, buf);
	}
}

void DrawingCanvas::onImageChanged() {
	markAsDirty(sBrushImagePathDirty);
}

} // namespace ui
} // namespace ds
