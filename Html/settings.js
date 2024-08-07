{
   var currentSettings;

   const xhr = new XMLHttpRequest();
   function ajaxHandler ()
   {
      if (xhr.readyState === XMLHttpRequest.DONE)
      {
         if (xhr.status === 200)
         {
            var data = JSON.parse (xhr.responseText);
            currentSettings = data;
            displaySettings ();
         }
         else
         {
            //alert('There was a problem with the request: ' + xhr.status);
         }
      }
   }

   function getSettings ()
   {
      if (window.location.protocol == "file:")
      {
         displaySettings ();
      }
      else
      {
         const url = 'getJson';
//         var formData = new FormData();
         var formData = new URLSearchParams ();
         formData.append ("request", "settings");

         xhr.open ('POST', url, true);
         xhr.setRequestHeader("Content-Type", "application/x-www-form-urlencoded");
         xhr.onreadystatechange = ajaxHandler;
         xhr.send (formData);
      }
   }


   function displaySettings ()
   {
      console.log (currentSettings);
      setElementValue ("ssid", currentSettings.ssid);
      setElementValue ("ipAddress", currentSettings.ipAddress);
      setElementValue ("netmask", currentSettings.netmask);
      setElementValue ("masterDevice", currentSettings.masterDevice);
      setElementValue ("username", currentSettings.username);
      setElementValue  ("currentDevice", currentSettings.currentDevice);
   }


   function setAsMaster ()
   {
      let id = document.getElementById ("currentDevice");
      if (id != null)
        setElementValue ("masterDevice", id.value);
   }
}
