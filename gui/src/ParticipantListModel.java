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
	
	public ParticipantListModel(Collection<String> initialParticipants, String leader) {
		participants = new HashSet<>(initialParticipants);
		if (leader != null && !participants.contains(leader)) {
			throw new IllegalArgumentException("Leader not in initValues list");
		}
		
		this.leader = leader;
		
		sortedParticipants = new LinkedList<>(initialParticipants);
		
		Collections.sort(sortedParticipants);
	}
	
	@Override
	public String getElementAt(int index) {
		String participant = sortedParticipants.get(index);
		return participant + (leader.equals(participant) ? " (leader)" : "");
	}

	@Override
	public int getSize() {
		return participants.size();
	}

	public void setParticipants(String[] participantsList, String leader) {
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
	
	public void addParticipant(String participant) {
		if (!participants.contains(participant)) {
			participants.add(participant);
			sortedParticipants.add(participant);
			Collections.sort(sortedParticipants);
			fireContentsChanged(this, 0, getSize() - 1);
		}
	}
	
	public void removeParticipant(String participant) {
		if (participants.contains(participant)) {
			if (participant.equals(leader)) {
				throw new IllegalArgumentException("Cannot remove leader");
			}
			participants.remove(participant);
			sortedParticipants.remove(participant);
			fireContentsChanged(this, 0, getSize() - 1);
		}
	}
	
	public void setLeader(String newLeader) {
		if (participants.contains(newLeader)) {
			leader = newLeader;
			int leaderIndex = sortedParticipants.indexOf(newLeader);
			fireContentsChanged(this,leaderIndex, leaderIndex);
		} else {
			throw new IllegalArgumentException("Leader not in model");
		}
	}
}
