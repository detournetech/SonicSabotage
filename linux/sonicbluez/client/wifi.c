/*
 *	ripped off from wireless-tools by Jean II
 */

#include "wifi.h"

static int get_essid(int skfd, const char * ifname, char * ssid_result)
{
  struct iwreq		wrq;
  char			essid[IW_ESSID_MAX_SIZE + 1];	/* ESSID */
  //unsigned int		i;
  //unsigned int		j;

  /* Make sure ESSID is always NULL terminated */
  memset(essid, 0, sizeof(essid));

  /* Get ESSID */
  wrq.u.essid.pointer = (caddr_t) essid;
  wrq.u.essid.length = IW_ESSID_MAX_SIZE + 1;
  wrq.u.essid.flags = 0;
  if(iw_get_ext(skfd, ifname, SIOCGIWESSID, &wrq) < 0)
    return(-1);

  //printf("%s\n", essid);
  memcpy(ssid_result, essid, sizeof(essid));
  return(0);
}

static int scan_devices(int skfd, char * ssid_result)
{
  char		buff[1024];
  struct ifconf ifc;
  struct ifreq *ifr;
  int		i;

  /* Get list of active devices */
  ifc.ifc_len = sizeof(buff);
  ifc.ifc_buf = buff;
  if(ioctl(skfd, SIOCGIFCONF, &ifc) < 0)
    {
      perror("SIOCGIFCONF");
      return(-1);
    }
  ifr = ifc.ifc_req;

  /* Print the first match */
  for(i = ifc.ifc_len / sizeof(struct ifreq); --i >= 0; ifr++)
    {
		if(get_essid(skfd, ifr->ifr_name, ssid_result) >= 0)
			return 0;
    }
  return(-1);
}

int domain(char * ssid_result)
{
  int	skfd;			/* generic raw socket desc.	*/
  //int	format = FORMAT_DEFAULT;
  //int	wtype = WTYPE_ESSID;
  int	opt;
  int	ret = -1;

  /* Create a channel to the NET kernel. */
  if((skfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
  {
    perror("socket");
    return(-1);
  }

  ret = scan_devices(skfd, ssid_result);

  close(skfd);
  return(ret);
}
