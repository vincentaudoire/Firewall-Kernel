#include <linux/module.h> 
#include <linux/kernel.h>
#include <linux/proc_fs.h> 
#include <asm/uaccess.h>
#include <linux/list.h>
#include <linux/netfilter.h> 
#include <linux/netfilter_ipv4.h>
#include <linux/skbuff.h>
#include <linux/compiler.h>
#include <linux/smp_lock.h>
#include <net/tcp.h>
#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/interrupt.h>
MODULE_AUTHOR ("<vxa125@bham.ac.uk>");
MODULE_LICENSE("GPL");

#define BUFFERLENGTH sizeof(char)+2*sizeof(int)
#define BUFFERSIZE 1024
#define ENV_BUFFER 4096

#define ADD 'A'
#define DELETE 'D'

DECLARE_RWSEM(list_sem); 

struct proc_dir_entry *procKernel; 
struct nf_hook_ops *reg;


int counter = 0; 

struct capability{
int port;
int capability;
};

struct capabilitiesList {
int port;
int capability;
char rwd;
  struct capabilitiesList *next;
}; 

struct capabilitiesList *kernelList = NULL;

struct capabilitiesList *add_entry (struct capabilitiesList *capabilitiesList,int _capability, int _port) {

  struct capabilitiesList *newEntry;
  newEntry = kmalloc (sizeof (struct capabilitiesList), GFP_KERNEL);
  if (!newEntry) {
    return NULL;
  }
  newEntry->port = _port;
  newEntry->capability=_capability;
  down_write (&list_sem);
  newEntry->next = capabilitiesList;
  capabilitiesList = newEntry;
  up_write (&list_sem);
  return capabilitiesList;
  
}


void show_table (struct capabilitiesList *capabilitiesList) {
  struct capabilitiesList *tmp;
  down_read (&list_sem);
  tmp = capabilitiesList;
  while (tmp) {
    printk (KERN_INFO "kernelmodule:The next capability is %d %d\n", tmp->capability,tmp->port);
    tmp = tmp->next;
  }
  up_read (&list_sem); 

}

struct capabilitiesList * remove_entry (struct capabilitiesList *capabilitiesList,int _capability, int _port){
	struct capabilitiesList *tmp;
	struct capabilitiesList *tmp_before;
	int count=0;
	down_read (&list_sem);
	tmp = capabilitiesList;
	tmp_before=tmp;
	while(tmp){
		count++;
	         printk(KERN_INFO " %d %d \n",tmp->capability,tmp->port);
	        if(count==1){
			if(tmp->capability==_capability && tmp->port==_port) {
			tmp_before=tmp->next;
			kfree(tmp);
			up_read (&list_sem); 
			return tmp_before;
			}
		}
		else{
			if(tmp->capability==_capability && tmp->port==_port){
			tmp_before->next=tmp->next;
			kfree(tmp);
			up_read (&list_sem); 
			return kernelList;
			}
	}
	tmp_before=tmp;
	tmp=tmp->next;
	}
	up_read (&list_sem); 
	return kernelList;
} 



int kernelRead (struct file *file, const char *buffer, unsigned long count, void *data) { 
  char *kernelBuffer; 
  struct capabilitiesList *tmp;

  kernelBuffer = kmalloc (BUFFERLENGTH, GFP_KERNEL);
   
  if (!kernelBuffer) {
    return -ENOMEM;
  }

  if (count > BUFFERLENGTH) { 
    kfree (kernelBuffer);
    return -EFAULT;
  }


 
  if (copy_from_user (kernelBuffer, buffer, count)) { 
    kfree (kernelBuffer);
    return -EFAULT;
  }

	
  switch (((char*) kernelBuffer)[0]) {
    case ADD:


      tmp = add_entry (kernelList, *((int *)(kernelBuffer+sizeof(char))),*((int *)(kernelBuffer+sizeof(char)+sizeof(int))));
      if (!tmp) {
	kfree (kernelBuffer);
	return -EFAULT;
      }
      else {
	kernelList = tmp;
      }
      break;
    case DELETE:
      tmp=remove_entry(kernelList,*((int *)(kernelBuffer+sizeof(char))),*((int *)(kernelBuffer+sizeof(char)+sizeof(int))));
      if(!tmp){
           kernelList=tmp;
      	  kfree(kernelBuffer);
      }
      else {
      	kernelList=tmp;
      }
      break;
    default: 
      printk (KERN_INFO "kernelRead: Illegal command \n");
  }
  return count;
}
int kernelWrite (
		 char *buffer, char **start, off_t offset,int buffer_size, int *eof, void *data) {
  char *pos;    /* the current position in the buffer */
  struct capability capability;
  int retval = 0;  /* number of bytes read; return value for function */
  int i = 0;
  struct capabilitiesList *tmp=kernelList;
  printk (KERN_INFO "procfile_read called with offset of %d and buffer size %d\n", (int) offset, buffer_size);
  show_table(kernelList);
  pos = buffer;
	while(tmp){
	capability.port=tmp->port;
	capability.capability =tmp->capability;
	 printk (KERN_INFO "kernel WRITE port : %d capability : %d \n",tmp->port,tmp->capability);
	memcpy (pos, &capability, sizeof (struct capability));
	pos += sizeof (struct capability);
	counter++;
	i ++;
	retval = retval + sizeof (struct capability);
	tmp=tmp->next;
	}
	
  if (counter == BUFFERSIZE) {
    *eof = 1;
    counter = 0;
  }
  printk (KERN_INFO "procfile read returned %d byte\n", retval);
  return retval;
}

static ssize_t get_environ (char *buffer, size_t count) 
{
	struct mm_struct *mm;
	ssize_t ret = -ENOMEM;
	unsigned long src = 0;

	if (in_irq() || in_softirq() || !(mm = get_task_mm(current)) || IS_ERR (mm)) {
		printk (KERN_INFO "Not in user context - should not happen!!\n");
		return -EFAULT;
	}
		
	/* now in user context, and virtual memory available */
	ret = 0;
	if (count > 0) {
		int this_len;

		/* length of environment buffer in kernel */
		this_len = mm->env_end - (mm->env_start + src);

		/* make sure we fit into the allocated buffer */
		if (this_len <= 0)
			this_len = 0;

		this_len = (this_len > count) ? count : this_len;
		
		/* copy the environment over */
		memcpy(buffer + src, ((char *)mm->env_start) + src, this_len);

		ret = this_len;
	}
	mmput(mm);
	return ret;
}

unsigned int FirewallExtensionHook (unsigned int hooknum,struct sk_buff *skb,const struct net_device *in,const struct net_device *out,int (*okfn)(struct sk_buff *)) {
  struct sock *sk;
  struct inet_sock *inet;
  int env_capabilities;
  size_t index = 0;
  size_t count;
  size_t length;
  char *buffer_env;	
  struct capabilitiesList *tmp=kernelList;
  sk = skb->sk;
  if (!sk) {
    printk (KERN_INFO "firewall: netfilter called with empty socket!\n");;
    return NF_ACCEPT;
  }
  if (sk->sk_protocol == IPPROTO_UDP) {
    return NF_ACCEPT;
  }
	  
  if (sk->sk_protocol != IPPROTO_TCP) {
    printk (KERN_INFO "firewall: netfilter called with non-TCP-non-UDP-packet.\n");
    return NF_ACCEPT;
  }
  inet = inet_sk (sk);
  if (!inet) {
    printk (KERN_INFO "firewall: netfilter passed TCP-packet with bad header!\n");
    return NF_ACCEPT;
  }
 
  if (sk->sk_state == TCP_SYN_SENT) {
      printk (KERN_INFO "firewall: Starting connection \n");
      printk (KERN_INFO "firewall: Destination address = %u.%u.%u.%u, destination port = %d\n", NIPQUAD(inet->daddr), htons(inet->dport)); 
      printk(KERN_INFO "Loading module for reading from environment");
	buffer_env = kmalloc (ENV_BUFFER, GFP_KERNEL);
	if (!buffer_env) {
		return -ENOMEM;
	}
	memset (buffer_env, 0, ENV_BUFFER);
	count = get_environ (buffer_env, ENV_BUFFER);
	if (count < 0) {
		printk (KERN_INFO "Unable to read environment!\n");
		kfree (buffer_env);
		return -EFAULT;
	}
	buffer_env[ENV_BUFFER] = '\0';
	while ((index < count) && buffer_env[index]) {
		length = strlen (buffer_env + index);
		index = index + length + 1;
		if(strncmp(buffer_env + index,"CAPABILITIES=",13)==0){
			env_capabilities=(int)simple_strtol(buffer_env + index + 13,0,10);
			while(tmp){
			if(env_capabilities==tmp->capability){
				if(htons (inet->dport)==tmp->port){
					printk (KERN_INFO "connection accepted\n");
					kfree (buffer_env);
					return NF_ACCEPT;
				}
			}
			tmp=tmp->next;
			}
			kfree (buffer_env);
			printk (KERN_INFO "firewall kernel module : connection not allowed\n");
			tcp_done (sk);
			return NF_DROP;
		}
	}
	printk (KERN_INFO "firewall kernel module : connection not allowed\n");
      	kfree (buffer_env);
      	tcp_done (sk);
	return NF_DROP;

    }
  return NF_ACCEPT;	
}

EXPORT_SYMBOL (FirewallExtensionHook);

int init_module (void) {
     int errno;
   procKernel = create_proc_entry ("kernelSwap", S_IWUSR | S_IRUGO, NULL);

   if (!procKernel) {
     return -ENOMEM;
   }

   procKernel->read_proc = kernelWrite;
   procKernel->write_proc= kernelRead;

  reg = kmalloc (sizeof (struct nf_hook_ops), GFP_KERNEL);
  if (!reg) {
    return -ENOMEM;
  }

  reg->hook = FirewallExtensionHook; 
  reg->pf = PF_INET;
  reg->owner = THIS_MODULE;
  reg->hooknum = NF_INET_LOCAL_OUT;

  errno = nf_register_hook (reg); 
  if (errno) {
    printk (KERN_INFO "firewall kernel module could not be registered!\n");
    kfree (reg);
  } 
  else {
   printk (KERN_INFO "firewall kernel module loaded \n");
  }
  return errno;
}

void cleanup_module (void) {
  remove_proc_entry ("kernelSwap", NULL); 
  nf_unregister_hook (reg); 
  kfree (reg);
  printk (KERN_INFO "firewall kernel module removed \n");

}
