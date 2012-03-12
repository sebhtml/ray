<?xml version="1.0" encoding="UTF-8"?>

<xsl:stylesheet version="1.0"
xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

<xsl:template match="/">
  <html>
<head>

<style>

body{
font-family: Arial;
}

table{
margin: 40px;
}

th{
background-color:green;
color:white;
border: 1px solid black;

padding:15px;
}

td{
width: 200px;
padding:15px;
border: 1px solid black;

}




</style>

<title>
Sample: <xsl:value-of select="root/sample"/>
</title>

</head>
  <body>
    <h1>Sample: <xsl:value-of select="root/sample"/></h1>
    <h2>Demultiplexed biological abundances from BiologicalAbundances/<xsl:value-of select="root/searchDirectory"/>/BiologicalAbundances.xml</h2>
	<h2>Produced by Ray technologies</h2>
<div>
Total assembled k-mer observations:
   <xsl:value-of select="root/totalAssembledKmerObservations"/>
</div>

<table><caption>Detected sequences</caption><tbody>
<tr><th>File</th><th>Sequence</th><th>Demultiplexed k-mer observations</th><th>Proportion</th></tr>

      <xsl:for-each select="root/entry">

        <xsl:sort data-type="number" order="descending" select="demultiplexedKmerObservations" />

<xsl:if test="demultiplexedKmerObservations/text() != '0'">

<tr><td>
<xsl:value-of select="file"/></td><td>
<xsl:value-of select="name"/></td><td>

          <xsl:value-of select="demultiplexedKmerObservations"/> </td><td>
          	<xsl:value-of select="100* proportion"/>%</td></tr>


</xsl:if>

      </xsl:for-each>
</tbody></table>

  </body>
  </html>
</xsl:template>

</xsl:stylesheet> 
