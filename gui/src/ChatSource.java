import java.io.BufferedReader;
import java.io.IOException;
import java.util.ArrayList;

public abstract class ChatSource {
	
	private static final String NEWLINE = System.lineSeparator();

	protected ChatRunner view;

	public ChatSource(ChatRunner r) {
		this.view = r;
	}
	
	public abstract void sendMessage(String message);
	
	/**
	 * 
	 * @param r
	 * @return False if reached end of stream, true otherwise
	 * @throws IOException
	 */
	protected boolean interpretInput(BufferedReader r) throws IOException {
		String line = r.readLine();
		
		if (line == null) {
			return false;
		}
		
		if (line.equals("Succeeded, current users:")) {
			// have to read in current users
			
			// build notice to display after
			StringBuilder notice = new StringBuilder().append(line).append(NEWLINE);
			String userLine;
			String leader = null;
			ArrayList<String> participants = new ArrayList<>();
			while ((userLine = r.readLine()) != null && !userLine.isEmpty()) {
				String[] parts = userLine.split(" ");
				if (parts.length < 2) {
					System.err.println("Invalid participants line: " + userLine);
				} else {
					participants.add(parts[0]);
					if (leader == null) {
						leader = parts[0];
					}
					notice.append(userLine).append(NEWLINE);
				}
			}
			if (userLine == null) {
				return false;
			}
			view.setParticipants(participants.toArray(new String[0]), leader);
			view.displayNotice(notice.toString());
			return true;
		}
		
		int messageColonIndex = line.indexOf(":: ");
		
		if (messageColonIndex > 0) {
			// Display message "NICKNAME:: ..."
			view.displayMessage(line.substring(0, messageColonIndex),
					line.substring(messageColonIndex + 3));
			return true;
		}
		
		// handle NOTICEs
		if (line.startsWith("NOTICE ")) {
			String[] lineParts = line.split(" ");
			
			if (lineParts.length == 7 && line.endsWith(" left the chat or crashed")) {
				// Left chat
				view.removeParticipant(lineParts[1]);
			} else if (lineParts.length == 5 && lineParts[2].equals("joined") && lineParts[3].equals("on")) {
				// Joined chat
				view.addParticipant(lineParts[1]);
			} else {
				// unknown notice
				System.err.println("Unknown notice: " + line);
				return true;
			}
			
			// display notice
			view.displayNotice(line.substring(7));
			return true;
		}
		
		if (line.equals("Bye.")) {
			view.makeInactive();
			return true;
		}
		
		if (line.indexOf(" started a new chat, listening on ") > 0) {
			// message when a leader first starts a new chat
			String nickname = line.split(" ")[0];
			view.initAsLeader(nickname);
			view.displayNotice(line);
			return true;
		}
		
		// catch-all, display as notice
		view.displayNotice(line);
		return true;
	}

	/**
	 * This method should be called when the ChatRunner is ready to start receiving
	 * calls, i.e. its GUI is set up.
	 */
	public abstract void run();
}
