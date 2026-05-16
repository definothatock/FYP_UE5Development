Other available language: [简体中文 CN_sim](README_CN_sim.md) | [繁體中文 CN_tra](README_CN_tra.md)


# Project Abstract

  - Project Title:  
    _Lord of the Flies: Cooperative Multiplayer Resources Scavenging Game_
  - Internal code:  
    AR1 25-26

**Objectives:**  
A cooperative scavenging extraction game set in a post-apocalyptic world.

**Motivation:**  
Popularity of cooperative multiplayer: Sharing fun on social media; Emotional bonding with friends.

<img width="906" height="510" alt="image" src="https://github.com/user-attachments/assets/966bb0fa-2e26-4fe8-a0ad-7187e64cc898" />

<br>
<br>

## Main Features:

### - **Procedural Map Generation**  
> Newly generated map each round  
<img width="240" height="240" alt="Video Project 6" src="https://github.com/user-attachments/assets/2c641f5e-3f04-4cf2-8d4a-87233e2e9eb4" />
<br>
<br>

> Mini Puzzles & world events - tradeoff dynamics
<img width="320" height="240" alt="torch" src="https://github.com/user-attachments/assets/3bd1099d-a01c-4e68-bee5-31eea7eb36fe" />
<img width="243" height="159" alt="image" src="https://github.com/user-attachments/assets/f86212ea-892c-42ad-ac92-d3f950e49386" />
<img width="190" height="152" alt="image" src="https://github.com/user-attachments/assets/5210596c-caff-4aac-ad8a-aeeb85f3f8f9" />

<br>
<br>

### - **Multiplayer**  
> Supporting voice chat and Steam/LAN connection  
<img width="366" height="185" alt="image" src="https://github.com/user-attachments/assets/5c9118a9-83dd-4596-b88d-9f4eaee366cd" />

<br>
<br>

### - **Inventory System**  
> Spatial for resource management  
<img width="320" height="240" alt="inv" src="https://github.com/user-attachments/assets/efa119ca-9afa-41fd-ac78-9ee9d0c232de" />
<br>
<br>

> Interesting items  
<img width="426" height="240" alt="bounce" src="https://github.com/user-attachments/assets/3f45229d-bc18-4053-b5a7-d31ba75de34f" />
<img width="426" height="240" alt="lift" src="https://github.com/user-attachments/assets/7f3bc785-7df6-4abc-b199-3c4226c05ba3" />
<img width="426" height="240" alt="shock" src="https://github.com/user-attachments/assets/3c9d00de-7fd6-47bd-9587-e5bffc3409ce" />

<br>
<br>

### - **Engaging AI**  
> unique behaviours and memories  
<img width="426" height="240" alt="blast" src="https://github.com/user-attachments/assets/94abc362-bc76-4c4e-b412-45ad7c8d3441" />
<img width="426" height="240" alt="targeting" src="https://github.com/user-attachments/assets/cb955935-54e6-45f9-935e-70fa649e59d1" />
<img width="426" height="240" alt="sneaking" src="https://github.com/user-attachments/assets/dfa172d6-9c52-4cca-accf-86ef2cbe269b" />

<br>
<br>

### - **Stylized  Visual**  
<img width="734" height="339" alt="image" src="https://github.com/user-attachments/assets/52ae4caa-e420-4589-923f-c3cc250b153c" />
<img width="641" height="374" alt="image" src="https://github.com/user-attachments/assets/70f48a7f-db39-4a4f-9598-111c44595634" />

<br>
<br>

# Status: Paused
  - The project reached playable Alpha and scored A-/A. Academically it fulfilled its purpose.
  - Manpower shifts. After the academic submission, the number of members who decided to remain is not enough to maintain the initially planned system depth.

There might be small refinements in the future. However, due to limitations of Git, updates will not be reflected in this specific repo.

<br>

### Main Contributor (main role & contributions):

- **Andy, CHIU (Definothatock):**  
  Project Lead / System Programmer  
    _Initial Design; AI behavioural system; Multiplayer connection system; General game subsystems (e.g. generic health system)_
  
- **Tonny, LO:**  
  System Programmer / Scene Designer  
    _World Generation system and most of its surrounding features and subsystems._
  
- **Danny, HO:**  
  3C Programmer / Gameplay Programmer  
    _Most of the direct Player interactions; All custom modelling and animation; many smaller subsystems (e.g. mini games)_

### Other Contributor:
- **Belinda:** _Inventory System_

<br>

### External asset:
- Model: Sketchfab
- Sounds: Pixabay

<br>

# Repo Summary

### This project was NOT handled with Git, it was handled with PEFORCE. This Git repo exists for making the project publicly visible.
**Due to data size limitations of Git, the actual source code cannot be stored directly. Only some C++ source is provided here.**

Project source: https://mega.nz/file/JRkTBZjR#ruYwuswSobcet8J0qzZYCgseCZBzuDQ8WdVdt_JMLTA  
_Source zip contains important data only (cloud limit, running requires recompile and setup), up until the final official submission._

Project Build: https://drive.google.com/file/d/1JyHeKAsFSqjTNmDESFGndiOMVn2ZsTow/view?usp=drive_link  
_Build zip contains the last playable version (wired for Steam connection, multiplayer requires the same Steam downloading-server region)._



<br>

# Project Details

 ## External links to detailed documentations

Final Report: https://drive.google.com/file/d/1Z5-iRUh7Rwk64DlKsWoWm-pX60PgoRi7/view?usp=drive_link  

Final Presentation: https://canva.link/4eh446oulrhd68v  

Final Project Video: https://www.youtube.com/watch?v=pSdOJv4ez60&feature=youtu.be  

Initial Proposal: https://drive.google.com/file/d/1ZlZn71hoDVWVREwAHO6I64GH2-1oVaNV/view?usp=sharing  

Obsidian (Project Documentation): https://github.com/definothatock/definothatock_Obsidian_FYP_UE5Development  

Trello (Team task management): https://trello.com/b/qntsQkkn/fyp-main-board  

## Academic comment of the project

Initial Proposal
> I find the proposal well-organized and thorough. The literature survey comparing different games is good, with concrete observations about what each game gets wrong. The objectives are clear, and the methodology section goes deep -- covering resource management, game rules, multiplayer architecture, even communication trade-offs. Some pages feel wastefully empty, which is a minor presentation issue. Overall, a strong proposal. <br>
--mingxunz

Development Progress
> The amount of work done is substantial -- modular health/stamina systems (server-authoritative, which is good for anti-cheating), a spatial inventory framework, distinct creature AIs using UE5's State Tree, procedural terrain generation, multiplayer via Steam, and custom shaders for ambience. The item design stands out -- the Cute Gun, Cloud Generator, and Tablet are not just functional but designed with player psychology in mind. I appreciate the thoughtful design for each item. The VoIP section shows deep engineering effort. They honestly describe encountering a lifecycle race condition with UE's built-in VoIP and made the pragmatic decision to switch to a third-party plugin. <br>
--mingxunz

Final Delevery
> The students managed to make a playable and synchronized alpha version within a pretty tight time frame. It's very well done. The terrain generation system is very impressive while the autonomous enemy system is still developing but interesting. / <br>
The final report shows the students made tremendous amount of effort in making a deliverable game. The terrain system, the characters' movement and the autonomous enemy AI look solid. The report is thorough and professional. The drawback: the delivery looks a bit unpolished. Many elements other than the main winning goal (puzzles, etc) make the game looks messy. The students also mentioned their original goal was not achieved because the goal was too ambitious. <br>
--mingxunz




<br><br>

Special thanks to our Supervisor Prof. **Sunil Arya**, Reader Assistant Prof. **Mingxun ZHOU**, and Tutor Miss **Noor Liza MD ISA**.
