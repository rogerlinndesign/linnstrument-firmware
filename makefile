linnstrument: 
	/Applications/Arduino.app/Contents/MacOS/JavaApplicationStub -v --verify /Development/linnstrument-firmware/linnstrument-firmware.ino

upload:
	cp /Development/linnstrument/build/linnstrument.build/Debug/linnstrument.build/linnstrument-firmware.cpp.bin /Development/linnstrument/Updater/
	open /Development/linnstrument/Updater/LinnStrument\ Updater.app/
