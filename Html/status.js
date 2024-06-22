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

         //var formData = new FormData();
         var formData = new URLSearchParams ();
         formData.append ("request", "status");
         formData.append ("timeStamp", new Date ().toISOString());

         xhr.open ('POST', url, true);
         xhr.setRequestHeader("Content-Type", "application/x-www-form-urlencoded");
         xhr.onreadystatechange = ajaxHandler;
         xhr.send (formData.toString ());
      }
   }

   function displayStatus ()
   {
      setElementInnerText ("requests", currentStatus.legalShown);
      setElementInnerText ("accepted", currentStatus.legalAccepted);
      setElementInnerText ("banned", currentStatus.totalBanned);
      setElementInnerText ("lastActivity", currentStatus.lastActivity);

      setElementInnerText ("iPhoneCount", currentStatus.iPhoneCount);
      setElementInnerText ("androidCount", currentStatus.androidCount);

      setElementInnerText ("sdkVersion", currentStatus.sdkVersion);
      setElementInnerText ("bootVersion", currentStatus.bootVersion);
      setElementInnerText ("chip", currentStatus.chip);
      setElementInnerText ("cores", currentStatus.cores);
      setElementInnerText ("chipID", currentStatus.chipID);
      setElementInnerText ("cpuFreq", currentStatus.cpuFreq);
      setElementInnerText ("revision", currentStatus.revision);
      setElementInnerText ("voltage", currentStatus.voltage);
      setElementInnerText ("memoryFree", currentStatus.memoryFree);
      setElementInnerText ("sketchSize", currentStatus.sketchSize);
      setElementInnerText ("sketchFree", currentStatus.sketchFree);
      setElementInnerText ("flashSize", currentStatus.flashSize);
      setElementInnerText ("softApMac", currentStatus.softApMac);
      setElementInnerText ("softApIP", currentStatus.softApIP);
   }
}
