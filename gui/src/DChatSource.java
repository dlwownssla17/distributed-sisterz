import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStreamWriter;
import java.io.PrintWriter;

public class DChatSource extends ChatSource {

	private String hintAddress;
	private String nickname;
	private String cmd;
	private Process chatProcess;
	private PrintWriter out;

	/**
	 * 
	 * @param cmd The executable to run, i.e. dchat
	 * @param nickname The local user's nickname
	 * @param hintAddress The address if joining a chat, null otherwise
	 */
	public DChatSource(ChatRunner view, String cmd, String nickname, String hintAddress) {
		super(view);
		this.cmd = cmd;
		this.nickname = nickname;
		this.hintAddress = hintAddress;
	}
	
	@Override
	public void sendMessage(String message) {
		out.println(message);
		out.flush();
	}

	@Override
	public void run() {
		String[] args;
		
		if (hintAddress == null) {
			args = new String[]{cmd, nickname};
		} else {
			args = new String[]{cmd, nickname, hintAddress};
		}
		try {
			ProcessBuilder builder = new ProcessBuilder(args);
			chatProcess = builder.start();
		} catch (IOException e) {
			throw new IllegalStateException(e);
		}
		new Thread(new InputHandlerThread(chatProcess.getInputStream())).start();
		new Thread(new InputHandlerThread(chatProcess.getErrorStream())).start();
		
		out = new PrintWriter(new OutputStreamWriter(chatProcess.getOutputStream()));
	}
	
	private class InputHandlerThread implements Runnable {

		private InputStream inputStream;

		public InputHandlerThread(InputStream inputStream) {
			this.inputStream = inputStream;
		}

		@Override
		public void run() {
			try {
				BufferedReader r = new BufferedReader(new InputStreamReader(inputStream));
				
				while (interpretInput(r));
				
				view.makeInactive();

				r.close();
			} catch (IOException e) {
				e.printStackTrace();
				System.exit(1);
			}
		}
	}
}