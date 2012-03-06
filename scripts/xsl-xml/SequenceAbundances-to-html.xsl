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

.taxon{
border-style:solid; 
background: #FFFF33;

margin-top:50px;
margin-bottom:50px;
margin-right:50px;
margin-left:50px;

padding-top:30px;
padding-bottom:30px;
padding-right:30px;
padding-left:30px;
}

.name{
font-size: 24px;
}

.observations{

padding-top:10px;
padding-bottom:10px;
padding-right:10px;
padding-left:10px;
}

.proportion{
color: white;
background: black;
border-color: white;
font-size: 48px;

border-style:solid; 

padding-top:5px;
padding-bottom:5px;
padding-right:5px;
padding-left:5px;
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

      <xsl:for-each select="root/entry">


<xsl:if test="demultiplexedKmerObservations/text() != '0'">

<div class="taxon">
          <div>
		<span class="name">
<xsl:value-of select="file"/><br />
<xsl:value-of select="name"/>

</span>

		<xsl:text disable-output-escaping="yes">&amp;nbsp;</xsl:text>

          	<span class="proportion"><xsl:value-of select="100* proportion"/>%</span>
	</div>

<div class="observations">
Demultiplexed k-mer observations:
          <xsl:value-of select="demultiplexedKmerObservations"/> / <xsl:value-of select="/root/totalAssembledKmerObservations"/>
</div>

</div>

</xsl:if>

      </xsl:for-each>

  </body>
  </html>
</xsl:template>

</xsl:stylesheet> 
