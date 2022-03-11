   const xhr = new XMLHttpRequest();

   function expirationHandler ()
   {
      if (xhr.readyState === XMLHttpRequest.DONE)
      {
         if (xhr.status === 200)
         {
            let data = JSON.parse (xhr.responseText);
            expire.innerHTML = data.expire;
         }
         else
         {
            //alert('There was a problem with the request: ' + xhr.status);
         }
      }
   }


   function getExpirationTime ()
   {
      const url = 'getJson';
      var formData = new FormData();
      formData.append ("request", "expire");

      xhr.open ('POST', url, true);
      xhr.onreadystatechange = expirationHandler;
      xhr.send (formData);
   }
