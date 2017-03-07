/************************* ls_bytebuffer: LinnStrument ByteBuffer class ***************************
This work is licensed under the Creative Commons Attribution-ShareAlike 3.0 Unported License.
To view a copy of this license, visit http://creativecommons.org/licenses/by-sa/3.0/
or send a letter to Creative Commons, PO Box 1866, Mountain View, CA 94042, USA.
***************************************************************************************************
Circular byte buffer that has independent push and pop locations. This allows a fixed memory usage
for a queue of data, without having to worry about memory allocation.

Once the size of the buffer fills up though, new data will start filling the beginning back up
again. You need to be careful to select the approriate size of the byte buffer for your use-case
in order to prevent losing data.
**************************************************************************************************/

void selectSensorCell(byte col,             // column to be addressed by analog switches
                      byte row,             // row to be addressed by analog switches
                      byte switchCode);
