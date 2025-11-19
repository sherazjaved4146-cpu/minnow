Checkpoint 2 Writeup
====================

My name: [Muhammad Sheraz Javed,Ahmed hassan]

My SUNet ID: [23L-0591]

I collaborated with: [23L-0861]

I would like to thank/reward these classmates for their help: [list sunetids here]

This lab took me about [n] hours to do. I [did/did not] attend the lab session.

Describe Wrap32 and TCPReceiver structure and design. [Describe data
structures and approach taken. Describe alternative designs considered
or tested.  Describe benefits and weaknesses of your design compared
with alternatives -- perhaps in terms of simplicity/complexity, risk
of bugs, asymptotic performance, empirical performance, required
implementation time and difficulty, and other factors. Include any
measurements if applicable.
//for wrap around:
(1->wrap function)i made a one liner code which converts 64-bits to 32 bits and used type_casting{static_cast<uint32_t>(n)//when value is 2^32-1 -> 0}{benifits ->0(1) complexity 0(1) space complexity and simple}
(2->unwrap function)we converted 32-bits sequence packets to again 64 bits for our rescembler file(for application) for that we had 3 cases (better known -> canidate=offset+base) 1->lower canidate(canidate - 2^32) 2->canidate 3->canidate+2^32
these case are handeled with if else statement (2 wraped around and 1 not wraped)after comparing got the closest match with checkpoint variable that i declared and than returned that;

afterwards i handeled tecp_recieve files:
in which we have two two parts 1->recieve 2->send
1->in recieve part ->i just handeled rst flag in first line the capture syn bit if first segment if first segment did'nt come then simply ignored those packets or messages.
then unwraped from 32-bit seq no to 64 absolute seq number and finally pushes to reassembler 
2->in send part->we only need to send acks,window_size and setthe rst flag if error.
that's all
]

Implementation Challenges:
[->i got challenges in order to pass the 30 checks so that's why i used the help of gpt]

Remaining Bugs:
[-.no bugs all complete]

- If applicable: I received help from a former student in this class,
  another expert, or a chatbot or other AI system (e.g. ChatGPT,
  Gemini, Claude, etc.), with the following questions or prompts:
  [please list questions/prompts]

- Optional: I had unexpected difficulty with: [describe]

- Optional: I think you could make this lab better by: [describe]

- Optional: I was surprised by: [describe]

- Optional: I'm not sure about: [describe]

- Optional: I made an extra test I think will be helpful in catching bugs: [submit as GitHub PR
  and include URL here]
