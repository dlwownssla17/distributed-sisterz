import java.awt.BorderLayout;
import java.awt.Color;
import java.awt.Container;
import java.awt.Dimension;
import java.awt.Font;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.util.Collections;

import javax.swing.BorderFactory;
import javax.swing.JFrame;
import javax.swing.JLabel;
import javax.swing.JList;
import javax.swing.JOptionPane;
import javax.swing.JScrollPane;
import javax.swing.JTextField;
import javax.swing.JTextPane;
import javax.swing.UIManager;
import javax.swing.UnsupportedLookAndFeelException;
import javax.swing.text.BadLocationException;
import javax.swing.text.Document;
import javax.swing.text.SimpleAttributeSet;
import javax.swing.text.StyleConstants;

public class ChatRunner {
	
    private ParticipantListModel participantsModel;
	private SimpleAttributeSet noticeStyle;
	private SimpleAttributeSet nicknameStyle;
	private Document chatDoc;
	private JTextField chatEntry;
	private ChatSource source;
	private String nickname;
	private boolean startNewChat;
	private String hintAddress;

	private ChatRunner(String sourcePointer) {
		try {
            UIManager.setLookAndFeel(UIManager.getSystemLookAndFeelClassName());
        } catch (UnsupportedLookAndFeelException ex) {
            ex.printStackTrace();
        } catch (IllegalAccessException ex) {
            ex.printStackTrace();
        } catch (InstantiationException ex) {
            ex.printStackTrace();
        } catch (ClassNotFoundException ex) {
            ex.printStackTrace();
        }
        /* Turn off metal's use bold fonts */
        UIManager.put("swing.boldMetal", Boolean.FALSE);
        
		javax.swing.SwingUtilities.invokeLater(new Runnable() {
            public void run() {
                createAndShowGUI();
                //source = new MockSource(ChatRunner.this, sourcePointer);
                source = new DChatSource(ChatRunner.this, sourcePointer, nickname, hintAddress);
                source.run();
            }
        });
	}
	
	public static void main(String[] args) {
		if (args.length < 1) {
			System.out.println("Need input hint");
			System.exit(1);
		}
		
		new ChatRunner(args[0]);
	}

	private void createAndShowGUI() {
        //Create and set up the window.
        JFrame frame = new JFrame("Distributed Sisterz Chat");
        frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
        //Set up the content pane.
        addComponentsToPane(frame.getContentPane());
        //Use the content pane's default BorderLayout. No need for
        //setLayout(new BorderLayout());
        //Display the window.
        frame.pack();
        frame.setVisible(true);
        
        // prompt for input
        nickname = JOptionPane.showInputDialog(frame, "Choose a nickname:");
        
        if (nickname == null) {
        	System.exit(0);
        }
        
        startNewChat = JOptionPane.showOptionDialog(frame,
        		"Do you want to start a new chat or join an existing chat?",
        		"Distributed Sisterz", JOptionPane.YES_NO_OPTION, JOptionPane.QUESTION_MESSAGE,
        		null, new String[]{"Join Chat", "New Chat"}, 1) != 0;
        
        if (!startNewChat) {
        	hintAddress = JOptionPane.showInputDialog(frame,
        			"Enter the IP Address and Port Number of someone in the chat");
        }
        
        // create style for notice
        noticeStyle = new SimpleAttributeSet();
        StyleConstants.setForeground(noticeStyle, Color.GRAY);
        StyleConstants.setBold(noticeStyle, true);
        
        // create style for message nickname
        nicknameStyle = new SimpleAttributeSet();
        StyleConstants.setForeground(nicknameStyle, Color.BLUE);
        StyleConstants.setBold(nicknameStyle, true);
    }
	
	private void addComponentsToPane(Container pane) {
		if (!(pane.getLayout() instanceof BorderLayout)) {
            pane.add(new JLabel("Container doesn't use BorderLayout!"));
            return;
        }
		
		// create font
		Font defaultFont = new Font(Font.SANS_SERIF, Font.PLAIN, 14);
        
		// create text pane displaying chat history
        JTextPane chatHistoryPane = new JTextPane();
        
        chatHistoryPane.setFont(defaultFont);
        
        chatDoc = chatHistoryPane.getDocument();
        
        chatHistoryPane.setEditable(false);
        chatHistoryPane.setPreferredSize(new Dimension(400,400));
        
        JScrollPane scrollPane = new JScrollPane(chatHistoryPane);
        
        pane.add(scrollPane, BorderLayout.CENTER);
        
        // create participants list
        
        participantsModel = new ParticipantListModel(Collections.emptyList(), null);
        
        JList<String> participantsList = new JList<>(participantsModel);
        participantsList.setBorder(BorderFactory.createLineBorder(Color.black));
        participantsList.setPreferredSize(new Dimension(150,400));
        participantsList.setFont(defaultFont);
        
        pane.add(participantsList, BorderLayout.LINE_START);
        
        // create text entry
        
        chatEntry = new JTextField();
        chatEntry.setBackground(Color.WHITE);
        chatEntry.setFont(defaultFont);
        pane.add(chatEntry, BorderLayout.PAGE_END);
        
        // respond to hitting ENTER in the text box
        chatEntry.addActionListener(new ActionListener() {
			@Override
			public void actionPerformed(ActionEvent e) {
				source.sendMessage(chatEntry.getText());
				chatEntry.setText("");
			}
		});
	}

	/*
	 * Methods for updating chat state
	 */
	
	public void displayMessage(String sender, String message) {
		try {
			chatDoc.insertString(chatDoc.getLength(), sender + ": ", nicknameStyle);
			chatDoc.insertString(chatDoc.getLength(), message + "\n", null);
		} catch (BadLocationException e) {
			e.printStackTrace();
		}
	}
	
	/**
	 * Displays a notice in the chat window, as-is.
	 * 
	 * @param notice
	 */
	public void displayNotice(String notice) {
		try {
			chatDoc.insertString(chatDoc.getLength(), notice + "\n", noticeStyle);
		} catch (BadLocationException e) {
			e.printStackTrace();
		}
	}
	
	/**
	 * Adds a participant to the list of participants
	 * 
	 * @param name
	 */
	public void addParticipant(String name) {
		participantsModel.addParticipant(name);
	}
	
	/**
	 * Removes a participant from the set of participants
	 * 
	 * @param name
	 */
	public void removeParticipant(String name) {
		participantsModel.removeParticipant(name);
	}
	
	/**
	 * Sets the list of participants
	 * @param names All of the names of participants
	 * @param leader The name of the leader, must be included in list of names
	 */
	public void setParticipants(String[] names, String leader) {
		participantsModel.setParticipants(names, leader);
	}

	/**
	 * Grays out the text entry
	 */
	public void makeInactive() {
		chatEntry.setEnabled(false);
	}

	public void initAsLeader(String nickname) {
		participantsModel.addParticipant(nickname);
		participantsModel.setLeader(nickname);
	}
}
