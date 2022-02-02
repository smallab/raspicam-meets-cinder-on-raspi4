# Extension du Vide
["Extension du Vide"](https://www.mariejuliebourgeois.fr/projets/extension-du-vide/) is an open-sourced art work by French artist Marie-Julie Bourgeois.

![Extension du Vide](https://www.mariejuliebourgeois.fr/wp-content/uploads/2021/10/EDV5.png)

## Required devices
* a Raspi board: [Raspberry Pi 4 Model B (preferably with 8Gb of RAM)](https://www.raspberrypi.com/products/raspberry-pi-4-model-b/)
* a micro SD card of 8Gb minimum
* a power outlet for the Raspberry Pi
* a Raspicam, preferably infrared-enhanced: [Raspberry Pi Camera Module 2 NoIR](https://www.raspberrypi.com/products/pi-noir-camera-v2/)
* a video projector with a resolution of at least 1024 x 576px

## Raspbian OS setup

Start by installing Buster (v10 of Raspbian OS) and no other! Follow the link: https://www.raspberrypi.com/software/

Then do not forget to update & upgrade with the `apt-get` command:

```
sudo apt-get update
sudo apt-get upgrade
```

________________________________


## OpenCV 3.2

`sudo apt-get install libopencv-dev`

It must be v3.2.0 for this program.

## Raspicam

Start by allowing the use of the camera in `sudo raspi-config` : Interfaces/Camera…

Of interest for RaspiCam:
https://qengineering.eu/install-gstreamer-1.18-on-raspberry-pi-4.html

How the approach unfolded:
https://discourse.libcinder.org/t/capture-with-raspbery-pi-camera-module/1578
+
http://www.uco.es/investiga/grupos/ava/node/40
+
https://github.com/rmsalinas/raspicam
+

```
cmake ..
make
sudo make install
sudo ldconfig   
```

Finally, [update the lib path](https://raspberrypi.stackexchange.com/questions/24394/raspicam-c-api-mmal-linking):

`export LIBRARY_PATH=/opt/vc/lib`

When this is all setup, Cinder samples should compile with GCC.



______________________________

Please note: to simply run a previously compiled app on a different SD card and copy-pasted to a new SD card, that’s all that needs to be installed on that new SD.
In such case, copy the folder with BasicApp, resources folder and assets folder + make the BasicApp executable, as in:
`sudo chmod -x /Desktop/MJBEDV/BasicApp`

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



APP AUTOSTART
______________________________

[Auto-run C program on start-up - Raspberry Pi Forums](https://forums.raspberrypi.com/viewtopic.php?t=126937)
+
Some pimping from IVVS

`sudo apt-get install wmctrl`

`nano ~/Desktop/startup.sh`

```
#!/bin/sh
echo MJBEDV
cd /home/pi/Desktop/MJBEDV
./BasicApp
```

`sudo chmod +x ~/Desktop/startup.sh`


`nano ~/Desktop/focus.sh`

```
#!/bin/sh
wmctrl -R BasicApp

sudo chmod +x ~/Desktop/focus.sh
```


`sudo nano /etc/xdg/lxsession/LXDE-pi/autostart`

```
@lxpanel —profile LXDE-pi
@pxmanfm —desktop —profile LXDE-pi
#@xscreensaver -no-splash
sleep 5
/home/pi/Desktop/startup.sh
sleep 10
/home/pi/Desktop/focus.sh
```

`sudo reboot`

——————————————————
TODO

• utiliser la nouvelle 3D ajustée
• essayer de moins remonter la caméra quand la personne est très lointaine : à peine au-dessus de l’horizon
• test de flou
• diminuer le rendu (1440 à passer en 1280 ou même 1024) pour essayer d’optimiser le timing 
