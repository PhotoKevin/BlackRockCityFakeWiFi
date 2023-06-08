   const xhrBanned = new XMLHttpRequest();

   function expirationHandler ()
   {
      if (xhrBanned.readyState === XMLHttpRequest.DONE)
      {
         if (xhrBanned.status === 200)
         {
            let data = JSON.parse (xhrBanned.responseText);
            expire.innerHTML = data.expire;
         }
         else
         {
            //alert('There was a problem with the request: ' + xhrBanned.status);
         }
      }
   }


   function getExpirationTime ()
   {
      const url = 'getJson';
//      var formData = new FormData();
      var formData = new URLSearchParams ();
      formData.append ("request", "expire");

      xhrBanned.open ('POST', url, true);
      xhrBanned.setRequestHeader("Content-Type", "application/x-www-form-urlencoded");
      xhrBanned.onreadystatechange = expirationHandler;
      xhrBanned.send (formData);
   }

