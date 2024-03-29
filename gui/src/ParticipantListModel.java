import java.util.Collection;
import java.util.Collections;
import java.util.HashSet;
import java.util.LinkedList;
import java.util.List;
import java.util.Set;

import javax.swing.AbstractListModel;

public class ParticipantListModel extends AbstractListModel<String> {
	
	private static final long serialVersionUID = -1227570509839963277L;

	private Set<String> participants;
	
	private List<String> sortedParticipants;
	
	private String leader;

	private String ownNickname;
	
	public ParticipantListModel(Collection<String> initialParticipants, String leader) {
		participants = new HashSet<>(initialParticipants);
		if (leader != null && !participants.contains(leader)) {
			throw new IllegalArgumentException("Leader not in initValues list");
		}
		
		this.leader = leader;
		
		sortedParticipants = new LinkedList<>(initialParticipants);
		
		Collections.sort(sortedParticipants);
	}
	
	public void setOwnNickname(String ownNickname) {
		this.ownNickname = ownNickname;
	}

	@Override
	public synchronized String getElementAt(int index) {
		if (index >= sortedParticipants.size()) {
			return null;
		}
		String participant = sortedParticipants.get(index);
		return participant + (leader != null && leader.equals(participant) ? " (leader)" : "")
			+ (ownNickname.equals(participant) ? " (me)" : "");
	}

	@Override
	public synchronized int getSize() {
		return sortedParticipants.size();
	}

	public synchronized void setParticipants(String[] participantsList, String leader) {
		participants.clear();
		for (String p: participantsList) {
			participants.add(p);
		}
		if (!participants.contains(leader)) {
			throw new IllegalArgumentException("Leader not in participants list");
		}
		
		this.leader = leader;
		
		sortedParticipants.clear();
		sortedParticipants.addAll(participants);
		
		Collections.sort(sortedParticipants);
		
		fireContentsChanged(this, 0, getSize() - 1);
	}
	
	public synchronized void addParticipant(String participant) {
		if (!participants.contains(participant)) {
			participants.add(participant);
			sortedParticipants.add(participant);
			Collections.sort(sortedParticipants);
			fireContentsChanged(this, 0, getSize() - 1);
		}
	}
	
	public synchronized void removeParticipant(String participant) {
		if (participants.contains(participant)) {
			if (participant.equals(leader)) {
				leader = null;
			}
			participants.remove(participant);
			sortedParticipants.remove(participant);
			fireContentsChanged(this, 0, getSize() - 1);
		}
	}
	
	public synchronized void setLeader(String newLeader) {
		if (participants.contains(newLeader)) {
			int oldleaderIndex = sortedParticipants.indexOf(leader);
			leader = newLeader;
			int leaderIndex = sortedParticipants.indexOf(newLeader);
			fireContentsChanged(this, leaderIndex, leaderIndex);
			fireContentsChanged(this, oldleaderIndex, oldleaderIndex);
		} else {
			throw new IllegalArgumentException("Leader not in model");
		}
	}
}
