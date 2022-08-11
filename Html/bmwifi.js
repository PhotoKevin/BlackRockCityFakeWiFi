function setElementInnerText(elementID, innerText)
{
   let id = document.getElementById(elementID);
   if (id != null)
      id.innerText = innerText;
}

function setElementValue (elementID, value)
{
   let id = document.getElementById (elementID);
   if (id != null)
      id.value = value;
}


function getCookie (cName)
{
   const name = cName + "=";
   console.log (document.cookie);
   const cDecoded = decodeURIComponent (document.cookie);
   console.log (cDecoded);
   const cArr = cDecoded.split('; ');
   let res;
   cArr.forEach (val =>
   {
      if (val.indexOf(name) === 0) res = val.substring (name.length);
   })
   return res
}

const xhr = new XMLHttpRequest();
function getTitleHandler ()
{
   if (xhr.readyState === XMLHttpRequest.DONE)
   {
      if (xhr.status === 200)
      {
         var data = JSON.parse (xhr.responseText);
         document.title = data.title;
         setElementInnerText ("top", data.title);
      }
      else
      {
         //alert('There was a problem with the request: ' + xhr.status);
      }
   }
}


function setTitle ()
{
   if (window.location.protocol != "file:")
   {
      const url = 'getJson';
      var formData = new URLSearchParams ();
      formData.append ("request", "getTitle");

      xhr.open('POST', url, true);
      xhr.setRequestHeader ("Content-Type", "application/x-www-form-urlencoded");
      xhr.onreadystatechange = getTitleHandler;

      xhr.send (formData);
   }
}
