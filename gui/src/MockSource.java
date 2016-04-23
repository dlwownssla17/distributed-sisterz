import java.io.BufferedReader;
import java.io.FileReader;
import java.io.IOException;

public class MockSource extends ChatSource {

	private String filename;
	
	public MockSource(ChatRunner view, String filename) {
		super(view);
		this.filename = filename;
	}
	
	@Override
	public void sendMessage(String message) {
		view.displayMessage("THIS", message);
	}
	
	@Override
	public void run() {
		new Thread(new InputHandlerThread()).start();
	}
	
	private class InputHandlerThread implements Runnable {
		@Override
		public void run() {
			try {
				BufferedReader r = new BufferedReader(new FileReader(filename));
				
				while (interpretInput(r)) {
					// wait for 1 second between lines
					Thread.sleep(1000);
				}

				r.close();
			} catch (IOException | InterruptedException e) {
				e.printStackTrace();
				System.exit(1);
			}
			
		}
	}
}
