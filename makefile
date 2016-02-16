linnstrument: 
	/Applications/Arduino.app/Contents/MacOS/JavaApplicationStub -v --verify /Development/linnstrument-firmware/linnstrument-firmware.ino

upload:
	cp /Development/Linnstrument-XCode/build/linnstrument.build/Debug/linnstrument.build/linnstrument-firmware.cpp.bin /Development/Linnstrument-XCode/Updater/
	open /Development/Linnstrument-XCode/Updater/LinnStrument\ Updater.app/
