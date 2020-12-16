package cs149;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.RandomAccessFile;
import java.nio.MappedByteBuffer;
import java.nio.channels.*;
import java.nio.Buffer;
import java.nio.ByteBuffer;
import java.nio.channels.FileChannel;

public class BigBag {
	
	public static void main (String[] args) throws IOException  {
		for (String s : args) {
			System.out.println(s);
		}
		if (args.length < 2) { 
			System.out.println("Improper usage");
			System.exit(1);
		}
		

		RandomAccessFile file = new RandomAccessFile("src/test.dat", "r");
//		System.out.println("wow i made it here :)");
		long position = 0;
		long length = file.getChannel().size();
		int count = 0;
		while (position < length) {
			long remaining = length - position;
			long bytestomap = (long) Math.min(200, remaining);
			MappedByteBuffer mappedBuffer = file.getChannel().map(FileChannel.MapMode.READ_ONLY, position, bytestomap);
//			long limit = mappedBuffer.limit();
//			int count = 0;
			System.out.println("new line");
			while (mappedBuffer.hasRemaining()) {
				byte currentByte = mappedBuffer.get();
				if (count == 50) {
					count = 0;
					break;
				}
				count++;
				System.out.print(currentByte);
			}
		}
		
//        byte[] data = new byte[100];
//        while(mappedBuffer.hasRemaining()) {
//        	if (mappedBuffer.remaining() < remaining) {
//        		remaining = mappedBuffer.remaining();
//        	}
//        	mappedBuffer.get(data, 0, remaining);
//        	System.out.println(data);
//        }
	      
	}
}
