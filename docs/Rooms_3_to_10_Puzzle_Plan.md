# Rooms 3–10: proposed progression

These layouts use one saved ghost and the current 20-second recording limit. Only the security-beam actor is implemented in this change; room placement and playtesting are still required. Tutorial text and audio are user-authored.

| Room | Challenge | Recording the player makes |
| --- | --- | --- |
| 3 | Learn ghost shielding with one beam and its linked exit gate. | Walk into the beam and finish standing there. |
| 4 | Reach a crossing away from the useful blocking spot. | Move to a visible marked spot in the same beam; stop there so the player can take the alternate route. |
| 5 | Shield a beam while the player presses two buttons at once with a recorded press. | Stand in the beam near button A and press E. Player crosses safely and presses B at the recorded time. The button group controls the exit gate; leave the beam's Gate Ref empty. |
| 6 | Use the familiar pressure plate to reach a simultaneous button pair. | Stand on a plate near button A, press it, and stay. Player goes through the plate-controlled passage to press B. Use separate passage and exit gates, each with one driver. |
| 7 | Cross two beams in sequence. | Stand in beam A briefly, then move into beam B and finish. Player crosses A while shielded, then B. Only beam B drives the exit gate. |
| 8 | Two pairs of simultaneous buttons along a short route. | Press the first pair's A, move to the second pair's A, then press it. Player supplies both matching B presses. Each existing group controls its own gate. |
| 9 | Leave a plate and move into a beam while the player advances. | Hold a passage plate long enough for the player to cross, then walk to the beam and stop. Beam controls the exit gate. |
| 10 | Finale combining the previous interactions. | Hold a passage plate, move to the final beam beside button A, press A, then stay. Player crosses the passage and beam and presses B. Button group drives the final gate; beam Gate Ref remains empty. |

Target recordings of 8–15 seconds, leaving margin below 20. Keep paths short, show each relevant position from the starting area, and leave enough time to walk through moving gates. Increase the button group's timing window only if playtesting proves 1.5 seconds too strict.

Build and playtest Room 3 first, then Room 4. Before duplicating room actors, check each gate has a single driver and each exit zone has the correct source room number and next checkpoint. Every beam links to the same replay controller and gets its own Room Number. Do not copy the shared controller into additional rooms.
