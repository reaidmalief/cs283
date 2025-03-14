1. How does the remote client determine when a command's output is fully received from the server, and what techniques can be used to handle partial reads or ensure complete message transmission?

 > **Answer**: The remote client determines when a command's output is fully received by using an end-of-message marker, such as the EOF character (0x04). The server sends this marker after completing the command's output. To handle partial reads, the client reads data in chunks and checks for the EOF marker at the end of each chunk. If the marker isn't found, the client continues reading until it detects the EOF. This ensures complete message transmission even if the data is split across multiple TCP packets.

2. This week's lecture on TCP explains that it is a reliable stream protocol rather than a message-oriented one. Since TCP does not preserve message boundaries, how should a networked shell protocol define and detect the beginning and end of a command sent over a TCP connection? What challenges arise if this is not handled correctly?

 > **Answer**: A networked shell protocol can define message boundaries by using delimiters (e.g., a special character like \0 or 0x04) or by prefixing each message with its length. For example, the server can send the command length followed by the command itself. If this isn't handled correctly, the client might misinterpret where one command ends and another begins, leading to incomplete or garbled data. This can cause errors in command execution or even crash the shell.

3. Describe the general differences between stateful and stateless protocols.

 > **Answer**: Stateful protocols remember previous interactions, so they maintain context or session information between messages—think of it like a conversation where each reply builds on what was said before. Stateless protocols, on the other hand, treat every request as completely independent; every message starts with a clean slate. This makes stateless systems simpler and easier to scale, but sometimes it means more work is needed from the client to provide all the necessary details with each request.

4. Our lecture this week stated that UDP is "unreliable". If that is the case, why would we ever use it?

 > **Answer**:Even though UDP doesn’t guarantee that every packet arrives in order (or at all), it’s much faster and has less overhead than TCP. This makes UDP ideal for applications where speed is crucial and occasional data loss is acceptable—like streaming video, online gaming, or voice calls. In these cases, a little lost data won’t ruin the experience, but delays definitely would.

5. What interface/abstraction is provided by the operating system to enable applications to use network communications?

 > **Answer**:The OS provides the socket API, which is a standardized interface that lets applications create connections, send, and receive data over the network. Sockets abstract away the details of the underlying network hardware and protocols, allowing developers to focus on sending and receiving data in a consistent way.