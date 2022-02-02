# Extension du Vide
[Extension du Vide](https://www.mariejuliebourgeois.fr/projets/extension-du-vide/) is an open-sourced art work by French artist [Marie-Julie Bourgeois](https://www.mariejuliebourgeois.fr).

![Extension du Vide](https://www.mariejuliebourgeois.fr/wp-content/uploads/2021/10/EDV5.png)

## Required devices
* a "Pi" board: [Raspberry Pi 4 Model B (preferably with 8Gb of RAM)](https://www.raspberrypi.com/products/raspberry-pi-4-model-b/)
* a micro SD card of 8Gb minimum
* a power outlet for the Raspberry Pi
* a "RaspiCam", preferably infrared-enhanced because they work better in low light conditions: [Raspberry Pi Camera Module 2 NoIR](https://www.raspberrypi.com/products/pi-noir-camera-v2/)
* a video projector with a resolution of at least 1024 x 576px

## Raspbian OS setup

Start by installing Buster (v10 of Raspbian OS) and not the lastest! Follow the link: https://www.raspberrypi.com/software/. Obiously choose the GUI version… you need to display something in the end.

Then update & upgrade packages with the `apt-get` command:

```
sudo apt-get update
sudo apt-get upgrade
```

## OpenCV 3.2

`sudo apt-get install libopencv-dev`

It must be v3.2.0 for this program. Same here, don't try and get the latest. That's all there is to it, and if this works fine you're almost there.

## RaspiCam

### Plug & play
![](https://projects-static.raspberrypi.org/projects/getting-started-with-picamera/7ab130979e77e11eb977625713823e41ebe1ae64/en/images/pi4-camera-port.png)

Start by **allowing the use of the special camera interface on the Pi**. To do so via the Terminal, type `sudo raspi-config` and then choose Interfaces, Camera, and say that you want it enabled. Via the GUI open the raspberry menu, choose Preferences, Interfaces, Camera and click on the "enabled" radio button. You will need to `sudo reboot` for these changes to take effect.

If you want to learn more about the official Pi camera and its use, follow the [official RaspiCam guide](https://projects.raspberrypi.org/en/projects/getting-started-with-picamera/1).

### Use with C++

A research group specialized in AI and computer vision has created a very [neat library that lets us use the RaspiCam in a C++ project](http://www.uco.es/investiga/grupos/ava/node/40), with OpenCV involved or not.

Start by downloading the code that you will need to compile from [their SourceForge repository](https://sourceforge.net/projects/raspicam/files/?).

Unzip that file to a folder in your Documents folder (well that's up to you after all), fire up a Terminal window, navigate to that newly created folder with `cd ~/Documents/your_complicated_path_to_raspicam` and then `mkdir build && cd build && cmake ..`.

If that all worked, you're ready to compile the raspicam lib. Go for it:

```
make
sudo make install
sudo ldconfig   
```

Finally, [update the lib path](https://raspberrypi.stackexchange.com/questions/24394/raspicam-c-api-mmal-linking):

`export LIBRARY_PATH=/opt/vc/lib`

When this is all set up, Cinder samples should compile with GCC.



______________________________

Please note: to simply run an app that was previously compiled on a specific Pi (i.e. your dev Pi) and copy-pasted to a new Pi (i.e. your target, production Pi), that’s all that needs to be installed on that new Pi.
______________________________



## CINDER

https://libcinder.org/docs/branch/master/guides/linux-notes/rpi3.html
+
https://discourse.libcinder.org/t/running-on-a-rapberry-pi-4-solved/1595/2
+
https://github.com/go-vgo/robotgo/issues/19
+
https://stackoverflow.com/questions/61953106/compilation-fail-with-stdfilesystem-on-raspberry?noredirect=1&lq=1


`sudo apt-get install cmake`

```
sudo apt-get install libxcursor-dev \
libgles2-mesa-dev \
zlib1g-dev \
libfontconfig1-dev \
libmpg123-dev \
libsndfile1 \
libsndfile1-dev \
libpulse-dev \
libasound2-dev \
libcurl4-gnutls-dev \
libgstreamer1.0-dev \
libgstreamer-plugins-bad1.0-dev \
libgstreamer-plugins-base1.0-dev \
gstreamer1.0-libav \
gstreamer1.0-alsa \
gstreamer1.0-pulseaudio \
gstreamer1.0-plugins-bad \
libboost-filesystem-dev
```

```
sudo apt install libxrandr-dev \
libxinerama-dev \
libxi-dev
```

`sudo apt-get install xcb libxcb-xkb-dev x11-xkb-utils libx11-xcb-dev libxkbcommon-x11-dev`
`sudo apt-get install libxtst-dev`

`git clone —recursive https://github.com/cinder/Cinder.git`
`cd Cinder`
`mkdir build && cd build`
`cmake .. -DCINDER_TARGET_GL=es3-rpi`
`make -j 3`


Pour compiler sans erreur avec le system path, changer la ligne 136 dans le fichier proj/cmake/modules/cinderMakeApp.cmake (ajout du flag -lstdc++fs pour le Linker) :
	target_link_libraries( ${ARG_APP_NAME} PUBLIC cinder ${ARG_LIBRARIES} -lstdc++fs )


samples/BasicApp devrait compiler désormais avec son cmake


CINDER + RASPICAM
_________________________________

Dupliquer `Cinder/samples/BasicApp` puis ajouter de l’OpenCV qui exploite la raspicam

`mkdir build && cd build`
`cmake .. -DCINDER_TARGET_GL=es3-rpi`
Après un `cmake .. -DCINDER_TARGET_GL=es3-rpi` pour exploiter le bordel de raspicam il faut aller rajouter dans `samples/XXXXXXApp/proj/cmake/build/CMakeFiles/XXXXXXApp.dir/link.txt` à  la fin de la ligne :
` -lraspicam -lraspicam_cv -lmmal -lmmal_core -lmmal_util -lopencv_core -lopencv_imgcodecs -lopencv_imgproc`
`make`
`./Debug/BasicApp/BasicApp`


À partir de là  tout devrait être possible.

Pour pouvoir récupérer les `cv::Mat` produits par OpenCV il faut recupérer la méthode faite pour Cinder `fromOcv()` et pour ça on peut copier la classe et la fonction inline suivantes depuis https://github.com/cinder/Cinder-OpenCV/blob/master/include/CinderOpenCV.h :

```
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
```

## App autostart
To run art pieces in art spaces where you're not sitting all day to monitor power cuts, you better create an autostart routine. Good thing that the Pi is built for that: it doesn't have a start button, it just starts as soon as it gets the right kind of electricity flowing in it veins.

Start by installing a window controller app: `sudo apt-get install wmctrl`.

Then create a first shell script with `nano ~/Desktop/startup.sh`, type the following and save:

```
#!/bin/sh
echo MJBEDV
cd /home/pi/Desktop/MJBEDV
./BasicApp
```

Don't forget to make that executable with `sudo chmod +x ~/Desktop/startup.sh`.

Create a second script with `nano ~/Desktop/focus.sh`, type the following and save:

```
#!/bin/sh
wmctrl -R BasicApp
```

Again, make that executable with `sudo chmod +x ~/Desktop/focus.sh`.


Finally create/open the autostart routine file with `sudo nano /etc/xdg/lxsession/LXDE-pi/autostart` and type and save:

```
@lxpanel —profile LXDE-pi
@pxmanfm —desktop —profile LXDE-pi
#@xscreensaver -no-splash
sleep 5
/home/pi/Desktop/startup.sh
sleep 10
/home/pi/Desktop/focus.sh
```

You're all set, ready to `sudo reboot` to give it a shot.

——————————————————

If you're planning on running your app on a different Pi than where you develop (i.e. a production Pi) make sure to copy-paste the compiled BasicApp and copy-paste the resources folder and the assets folder besides that BasicApp. Once pasted on the new Pi, make the BasicApp executable with `sudo chmod -x ~/Desktop/MJBEDV/BasicApp`.
