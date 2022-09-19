/**
 * @file BasicApp.cpp
 * 
 * @author Matthieu Savary (contact@smallab.org)
 * @brief 
 * @version 0.2
 * @date 2022-09-16
 *
 * @copyright Copyright (c) 2022 SMALLAB.ORG
 * 
 */


#include "Resources.h"

#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Fbo.h"

#include "cinder/gl/Texture.h"
#include "cinder/Surface.h"
#include "cinder/ImageIo.h"
#include "cinder/ip/Checkerboard.h"

#include "cinder/Arcball.h"
#include "cinder/CameraUi.h"
#include "cinder/Sphere.h"

#include "cinder/ObjLoader.h"

#include "cinder/Rand.h"

#include <ctime>
#include <iostream>

#include <raspicam/raspicam_cv.h>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include "opencv2/imgcodecs.hpp"

using namespace std;
using namespace cv;

using namespace ci;
using namespace ci::app;


class ImageSourceCvMat : public ImageSource {
  public:
	ImageSourceCvMat( const cv::Mat &mat )
		: ImageSource()
	{
		mWidth = mat.cols;
		mHeight = mat.rows;
		if( (mat.channels() == 3) || (mat.channels() == 4) ) {
			setColorModel( ImageIo::CM_RGB );
			if( mat.channels() == 4 )
				setChannelOrder( ImageIo::BGRA );
			else
				setChannelOrder( ImageIo::BGR );
		}
		else if( mat.channels() == 1 ) {
			setColorModel( ImageIo::CM_GRAY );
			setChannelOrder( ImageIo::Y );
		}
		
		switch( mat.depth() ) {
			case CV_8U: setDataType( ImageIo::UINT8 ); break;
			case CV_16U: setDataType( ImageIo::UINT16 ); break;
			case CV_32F: setDataType( ImageIo::FLOAT32 ); break;
			default:
				throw ImageIoExceptionIllegalDataType();
		}

		mRowBytes = mat.step;
		mData = reinterpret_cast<const uint8_t*>( mat.data );
	}

	void load( ImageTargetRef target ) {
		// get a pointer to the ImageSource function appropriate for handling our data configuration
		ImageSource::RowFunc func = setupRowFunc( target );
		
		const uint8_t *data = mData;
		for( int32_t row = 0; row < mHeight; ++row ) {
			((*this).*func)( target, row, data );
			data += mRowBytes;
		}
	}
	
	const uint8_t		*mData;
	int32_t				mRowBytes;
};

inline ImageSourceRef fromOcv( cv::Mat &mat )
{
	return ImageSourceRef( new ImageSourceCvMat( mat ) );
}


// We'll create a new Cinder Application by deriving from the App class.
class BasicApp : public App {
  public:
	void setup() override;

    void keyDown( KeyEvent event ) override;

	void update() override;
	void draw() override;
	void cleanup() override;


  private:
	void	loadObj( const DataSourceRef &dataSource );
	void	detectMovement();
	void	renderSceneToFbo();
	void	renderSceneFboForBlurPass();

    raspicam::RaspiCam_Cv _raspicam;
    cv::Mat _previmage, _diffimage, _drawing;
	cv::Point2f _centroid;
	static const int	CAPTURE_WIDTH = 320, CAPTURE_HEIGHT = 240; // 4/3 ratio

	gl::FboRef			mSceneFbo, mBlurFbo;
	static const int	FBO_WIDTH = 1024, FBO_HEIGHT = 576; // 16/9 ratio

	vec2			_currentRot;
    int                _reminderTimeOffset, _reminderTimeLimit = 180;

	TriMeshRef		mMesh;
	Sphere			mBoundingSphere;
	CameraPersp		mCam;
	gl::BatchRef	mBatch; 

	gl::GlslProgRef	mGlsl, mGlslSecondPass;
};

void prepareSettings( BasicApp::Settings* settings )
{
	settings->setMultiTouchEnabled( false );
}

void BasicApp::setup()
{
    //set raspicam params
    _raspicam.set( CV_CAP_PROP_FORMAT, CV_8UC1 ); // CV_32F
    //Open raspicam
    if (!_raspicam.open()) {cerr<<"Error opening the Raspicam module"<<endl;quit();}

	// 3D
#if defined( CINDER_GL_ES )
	mGlsl = gl::GlslProg::create( loadAsset( "shader_es2.vert" ), loadAsset( "shader_es2.frag" ) );
	mGlslSecondPass = gl::GlslProg::create( loadAsset( "shader_es2.vert" ), loadAsset( "shader_es2.frag" ) );
#endif
	mGlsl->uniform( "tex0", 0 );
	mGlsl->uniform( "sample_offset", vec2( 1.0f / FBO_WIDTH, 0.0f ) );
	mGlsl->uniform( "attenuation", 1.0f );
	mGlslSecondPass->uniform( "tex0", 0 );
	mGlslSecondPass->uniform( "sample_offset", vec2( 0.0f, 1.0f / FBO_HEIGHT ) );
	mGlslSecondPass->uniform( "attenuation", 1.0f );

#if defined( CINDER_ANDROID )
	console() << "Loading object model" << std::endl;
	loadObj( loadFile( "surveillancecam.obj" ) );
#else
	loadObj( loadResource( RES_SURVEILLANCECAM_OBJ ) );
#endif

	// Scene Fbo
	//gl::Fbo::Format format; // depth tex is not supported on the Pi
	//format.setSamples( 4 ); // uncomment this to enable 4x antialiasing
	mSceneFbo = gl::Fbo::create( FBO_WIDTH, FBO_HEIGHT); //, format.depthTexture() );
	// Fbo 3D camera
	mCam.lookAt( vec3( 0.0f, 1.8f, -40.0f ), vec3( 0 ));
	// first blur pass Fbo
	mBlurFbo = gl::Fbo::create( FBO_WIDTH, FBO_HEIGHT); //, format.depthTexture() );

	// useless in Fbos on the Pi...
	gl::enableDepthRead();
	gl::enableDepthWrite();

	// fullscreen
	setFullScreen(true);

	// cursor
	hideCursor();

    // timer for reminder movements
    _reminderTimeOffset = getElapsedSeconds();
}

void BasicApp::keyDown( KeyEvent event )
{
	if( event.getChar() == 'o' ) {
		fs::path path = getOpenFilePath();
		if( ! path.empty() ) {
			loadObj( loadFile( path ) );
		}
	}
	else if( event.getChar() == 'f' ) {
		setFullScreen( !isFullScreen() );
	}
}

void BasicApp::loadObj( const DataSourceRef &dataSource )
{
	ObjLoader loader( dataSource );
	mMesh = TriMesh::create( loader );

	if( ! loader.getAvailableAttribs().count( geom::NORMAL ) )
		mMesh->recalculateNormals();

	mBatch = gl::Batch::create( *mMesh, gl::getStockShader( gl::ShaderDef().color() ) ); // solid color shader, will default to white

	mBoundingSphere = Sphere::calculateBoundingSphere( mMesh->getPositions<3>(), mMesh->getNumVertices() );
}

void BasicApp::update()
{
	// Movement detection
	detectMovement();

	// Fbo successive renderings
	renderSceneToFbo();
	renderSceneFboForBlurPass();
}

void BasicApp::detectMovement()
{
	// Do the raspicam grabbing & retrieving (according to previously set settings)
    cv::Mat image;
	_raspicam.grab();
	_raspicam.retrieve( image );

	if ( image.empty() ) return;

	// declare Mat variables
	cv::Mat thr, blr;

	// resize
	cv::resize(image, image, cv::Size(CAPTURE_WIDTH, CAPTURE_HEIGHT), 0, 0, cv::INTER_LINEAR);

	// diff (to get movement)
	_diffimage = image - _previmage;
	_previmage = image.clone();

	// blur before finding contours to remove noise
	cv::blur(_diffimage, blr, Size(32,32));

	// cut out more useless noise
	cv::threshold( blr, blr, 10,255, CV_THRESH_BINARY );

	// find contours
	vector<vector<Point> > contours;
	cv::findContours( blr, contours, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE );

	// get the moments
	vector<Moments> mu(contours.size());
	for( int i = 0; i<contours.size(); i++ )
	{ mu[i] = cv::moments( contours[i], false ); }

	// get the centroids of figures
	vector<Point2f> mc(contours.size());
	for( int i = 0; i<contours.size(); i++)
	{ mc[i] = Point2f( mu[i].m10/mu[i].m00 , mu[i].m01/mu[i].m00 ); }

    // reset time offset, something was moving in front of the camera
    if (contours.size() > 0) _reminderTimeOffset = getElapsedSeconds();

	// calculate pondered average centroid
	// using contour areas
	size_t counter = 0;
	if (contours.size() > 0) _centroid = Point2f(0, 0);
	for( size_t i = 0; i<contours.size(); i++ )
	{
		size_t surface = cv::contourArea(contours[i]);
		counter += (size_t)surface;
		_centroid.x += mc[i].x*surface;
		_centroid.y += mc[i].y*surface;
	}
	if (counter > 0) {
		_centroid.x /= counter;
		_centroid.y /= counter;
	}

    // if time limit is reached, move freely every 5 seconds
    if (_reminderTimeOffset + _reminderTimeLimit < getElapsedSeconds()) {
        _centroid = Point2f( Rand::randFloat(0, 200), Rand::randFloat(0, 200) );
        _reminderTimeOffset = getElapsedSeconds() - _reminderTimeLimit + 5;
    }
}

void BasicApp::renderSceneToFbo()
{
	// this will restore the old framebuffer binding when we leave this function
	// on non-OpenGL ES platforms, you can just call mSceneFbo->unbindFramebuffer() at the end of the function
	// but this will restore the "screen" FBO on OpenGL ES, and does the right thing on both platforms
	gl::ScopedFramebuffer fbScp( mSceneFbo );

	// clear out the FBO with pure black
	gl::clear( Color::black() );

	// setup the viewport to match the dimensions of the FBO
	gl::ScopedViewport scpVp( ivec2( 0 ), mSceneFbo->getSize() );

	// setup our camera to render the Fbo scene
	gl::setMatrices( mCam );

	// calculate our destination & step (velocity) angles
    float steps = 8.f;
    if (_centroid.x >= 0 && _centroid.y >= 0){
        vec2 dest = vec2(_centroid.x, _centroid.y);
        vec2 velocity = (dest - _currentRot) / vec2( steps );
        _currentRot += velocity;
    }

	// draw our surveillance cam batch
	gl::pushMatrices();
		gl::translate( vec3(0.0f, 5.0f, 0.0f) );
		gl::rotate(-(M_PI*0.375f) + (M_PI*0.75f) * _currentRot.x/CAPTURE_WIDTH, glm::vec3(0.0f, 1.0f, 0.0f));
		gl::rotate(-(M_PI*0.425f) + (M_PI*0.5f) * _currentRot.y/CAPTURE_HEIGHT, glm::vec3(1.0f, 0.0f, 0.0f));
		mBatch->draw();
	gl::popMatrices();
}

void BasicApp::renderSceneFboForBlurPass()
{
	// this will restore the old framebuffer binding when we leave this function
	// on non-OpenGL ES platforms, you can just call mSceneFbo->unbindFramebuffer() at the end of the function
	// but this will restore the "screen" FBO on OpenGL ES, and does the right thing on both platforms
	gl::ScopedFramebuffer fbScp( mBlurFbo );

	// clear out the FBO with green
	gl::clear( Color( 0.0f, 1.0f, 0.0f ) );

	// setup the viewport to match the dimensions of the FBO
	gl::ScopedViewport scpVp( ivec2( 0 ), mBlurFbo->getSize() );

	// show the FBO color texture starting in the upper left corner
	gl::ScopedTextureBind texScp( mSceneFbo->getColorTexture(), 0 );
	gl::setMatricesWindow( toPixels( mBlurFbo->getSize() ) );
	{
		gl::ScopedGlslProg shaderScp( mGlsl );
		gl::drawSolidRect( Rectf( 0, 0, FBO_WIDTH, FBO_HEIGHT ) );
	}
}

void BasicApp::draw()
{
	// clear the window to gray
	gl::clear( Color( 0.35f, 0.35f, 0.35f ) );

	// Final draw
	// show the FBO color texture starting in the upper left corner
	gl::setMatricesWindow( toPixels( getWindowSize() ) );
	{
		gl::ScopedTextureBind texScp( mBlurFbo->getColorTexture(), 0 );
		// throw in a second blur pass directly here
		gl::ScopedGlslProg shaderScp( mGlslSecondPass );
		gl::drawSolidRect( Rectf( 0, 0, getWindowWidth(), getWindowHeight() ) );
	}
}

void BasicApp::cleanup()
{
	_raspicam.release();
}

// This line tells Cinder to actually create and run the application.
CINDER_APP( BasicApp, RendererGl, prepareSettings )
