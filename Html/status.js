{
var questionSet;
var currentStatus;

   const xhr = new XMLHttpRequest();
   function ajaxHandler ()
   {
      if (xhr.readyState === XMLHttpRequest.DONE)
      {
         if (xhr.status === 200)
         {
            var data = JSON.parse (xhr.responseText);
            currentStatus = data;
            displayStatus ();
         }
         else
         {
            //alert('There was a problem with the request: ' + xhr.status);
         }
      }
   }



   function getStatus ()
   {
      if (window.location.protocol == "file:")
      {
         displayStatus ();
      }
      else
      {
         const url = 'getJson';
         var formData = new FormData();
         formData.append ("request", "status");

         xhr.open ('POST', url, true);
         xhr.onreadystatechange = ajaxHandler;
         xhr.send (formData);
      }
   }



   function displayStatus ()
   {
      document.getElementById ("requests").innerText = currentStatus.redirects;
      document.getElementById ("banned").innerText = currentStatus.banned;
      document.getElementById ("lastActivity").innerText = currentStatus.lastActivity;

      document.getElementById ("sdkVersion").innerText = currentStatus.sdkVersion;
      document.getElementById ("bootVersion").innerText = currentStatus.bootVersion;
      document.getElementById ("chipID").innerText = currentStatus.chipID;
      document.getElementById ("cpuFreq").innerText = currentStatus.cpuFreq;
//      document.getElementById ("cycleCount").innerText = currentStatus.cycleCount;
      document.getElementById ("voltage").innerText = currentStatus.voltage;
      document.getElementById ("memoryFree").innerText = currentStatus.memoryFree;
      document.getElementById ("sketchSize").innerText = currentStatus.sketchSize;
      document.getElementById ("sketchFree").innerText = currentStatus.sketchFree;
//      document.getElementById ("flashSpeed").innerText = currentStatus.flashSpeed;
      document.getElementById ("flashSize").innerText = currentStatus.flashSize;
      document.getElementById ("softApMac").innerText = currentStatus.softApMac;
      document.getElementById ("softApIP").innerText = currentStatus.softApIP;
      document.getElementById ("softApIP").innerText = jscd.os;

   }
}
